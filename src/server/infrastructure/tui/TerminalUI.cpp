/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TerminalUI implementation
*/

#include "infrastructure/tui/TerminalUI.hpp"
#include "infrastructure/tui/Utf8Utils.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace infrastructure::tui {

TerminalUI::TerminalUI(std::shared_ptr<LogBuffer> logBuffer)
    : _logBuffer(std::move(logBuffer))
{
    // Set callback for new log entries
    _logBuffer->setNewEntryCallback([this]() {
        if (_autoScroll) {
            _scrollOffset = 0;
        }
        _needsRefresh = true;
    });
}

TerminalUI::~TerminalUI() {
    stop();
}

void TerminalUI::start() {
    if (_running.exchange(true)) {
        return;  // Already running
    }

    TerminalRenderer::enableRawMode();
    TerminalRenderer::clearScreen();
    TerminalRenderer::hideCursor();

    _renderThread = std::jthread([this](std::stop_token stopToken) {
        while (!stopToken.stop_requested() && _running) {
            // Check for input
            int ch = TerminalRenderer::readChar();
            if (ch != -1) {
                processKeyInput(ch);
            }

            // Render if needed
            if (_needsRefresh.exchange(false)) {
                if (_mode == Mode::SplitScreen) {
                    renderSplitScreen();
                } else {
                    renderZoomMode();
                }
                TerminalRenderer::flush();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(RENDER_INTERVAL_MS));
        }
    });
}

void TerminalUI::stop() {
    if (!_running.exchange(false)) {
        return;
    }

    if (_renderThread.joinable()) {
        _renderThread.request_stop();
        _renderThread.join();
    }

    TerminalRenderer::showCursor();
    TerminalRenderer::resetScrollRegion();
    TerminalRenderer::disableRawMode();
    TerminalRenderer::clearScreen();
    TerminalRenderer::moveCursor(1, 1);
}

bool TerminalUI::isRunning() const {
    return _running;
}

void TerminalUI::setMode(Mode mode) {
    _mode = mode;
    _needsRefresh = true;
}

TerminalUI::Mode TerminalUI::getMode() const {
    return _mode;
}

void TerminalUI::setFilter(FilterLevel level) {
    _filterLevel = level;
    _scrollOffset = 0;
    _autoScroll = true;
    _needsRefresh = true;
}

TerminalUI::FilterLevel TerminalUI::getFilter() const {
    return _filterLevel;
}

void TerminalUI::scrollUp(size_t lines) {
    _autoScroll = false;
    auto maxScroll = _logBuffer->filteredSize(filterToSpdlogLevel(_filterLevel));
    _scrollOffset = std::min(_scrollOffset + lines, maxScroll > 0 ? maxScroll - 1 : 0);
    _needsRefresh = true;
}

void TerminalUI::scrollDown(size_t lines) {
    if (_scrollOffset <= lines) {
        _scrollOffset = 0;
        _autoScroll = true;
    } else {
        _scrollOffset -= lines;
    }
    _needsRefresh = true;
}

void TerminalUI::pageUp() {
    auto termSize = TerminalRenderer::getTerminalSize();
    scrollUp(termSize.rows / 2);
}

void TerminalUI::pageDown() {
    auto termSize = TerminalRenderer::getTerminalSize();
    scrollDown(termSize.rows / 2);
}

void TerminalUI::scrollToBottom() {
    _scrollOffset = 0;
    _autoScroll = true;
    _needsRefresh = true;
}

void TerminalUI::printToCommandPane(const std::string& text) {
    std::lock_guard<std::mutex> lock(_outputMutex);

    // Split by newlines
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        _commandOutput.push_back(line);
        if (_commandOutput.size() > MAX_COMMAND_OUTPUT) {
            _commandOutput.erase(_commandOutput.begin());
        }
    }
    _needsRefresh = true;
}

void TerminalUI::showPrompt(const std::string& prompt) {
    // The prompt is shown as part of renderCommandPane
    (void)prompt;  // Unused for now, prompt is hardcoded
    _needsRefresh = true;
}

std::string TerminalUI::processInputAndGetCommand() {
    std::lock_guard<std::mutex> lock(_inputMutex);
    std::string cmd = std::move(_completedCommand);
    _completedCommand.clear();
    return cmd;
}

void TerminalUI::refresh() {
    _needsRefresh = true;
}

void TerminalUI::requestRefresh() {
    _needsRefresh = true;
}

void TerminalUI::processKeyInput(int ch) {
    // Handle special keys
    if (ch == KEY_ESCAPE) {
        if (_mode == Mode::ZoomLogs) {
            setMode(Mode::SplitScreen);
        }
        return;
    }

    if (ch == KEY_UP) {
        scrollUp();
        return;
    }
    if (ch == KEY_DOWN) {
        scrollDown();
        return;
    }
    if (ch == KEY_PAGE_UP) {
        pageUp();
        return;
    }
    if (ch == KEY_PAGE_DOWN) {
        pageDown();
        return;
    }

    // Regular input for command line
    std::lock_guard<std::mutex> lock(_inputMutex);

    // Filter shortcuts - ONLY when not typing a command
    if (_inputBuffer.empty() && ch >= '1' && ch <= '4') {
        setFilter(static_cast<FilterLevel>(ch - '1'));
        return;
    }

    if (ch == '\n' || ch == '\r') {
        // Command completed
        _completedCommand = _inputBuffer;
        printToCommandPane("rtype> " + _inputBuffer);
        _inputBuffer.clear();
        _needsRefresh = true;
    } else if (ch == 127 || ch == '\b' || ch == 8) {  // Backspace
        if (!_inputBuffer.empty()) {
            _inputBuffer.pop_back();
            _needsRefresh = true;
        }
    } else if (ch >= 32 && ch < 127) {  // Printable ASCII
        _inputBuffer += static_cast<char>(ch);
        _needsRefresh = true;
    }
}

spdlog::level::level_enum TerminalUI::filterToSpdlogLevel(FilterLevel level) const {
    switch (level) {
        case FilterLevel::All:   return spdlog::level::trace;
        case FilterLevel::Info:  return spdlog::level::info;
        case FilterLevel::Warn:  return spdlog::level::warn;
        case FilterLevel::Error: return spdlog::level::err;
        default:                 return spdlog::level::trace;
    }
}

void TerminalUI::renderSplitScreen() {
    auto termSize = TerminalRenderer::getTerminalSize();
    uint16_t logPaneHeight = std::min(LOG_PANE_HEIGHT,
                                       static_cast<uint16_t>(termSize.rows - MIN_COMMAND_PANE_HEIGHT - STATUS_BAR_HEIGHT));
    uint16_t statusBarRow = logPaneHeight + 1;
    uint16_t commandPaneStart = statusBarRow + 1;
    uint16_t commandPaneHeight = termSize.rows - commandPaneStart + 1;

    renderLogPane(1, logPaneHeight);
    renderStatusBar(statusBarRow);
    renderCommandPane(commandPaneStart, commandPaneHeight);
}

void TerminalUI::renderZoomMode() {
    auto termSize = TerminalRenderer::getTerminalSize();
    renderLogPane(1, termSize.rows - 1);
    renderStatusBar(termSize.rows);
}

void TerminalUI::renderLogPane(uint16_t startRow, uint16_t height) {
    auto termSize = TerminalRenderer::getTerminalSize();
    auto spdlogLevel = filterToSpdlogLevel(_filterLevel);
    auto entries = _logBuffer->getFiltered(spdlogLevel);

    // Calculate visible range based on scroll offset
    size_t totalFiltered = entries.size();
    size_t visibleStart = 0;
    if (totalFiltered > height) {
        // _scrollOffset is from the bottom (0 = newest visible)
        size_t maxOffset = totalFiltered - height;
        size_t actualOffset = std::min(_scrollOffset, maxOffset);
        visibleStart = totalFiltered - height - actualOffset;
    }

    for (uint16_t row = 0; row < height; ++row) {
        TerminalRenderer::moveCursor(startRow + row, 1);
        TerminalRenderer::clearLine();

        size_t entryIdx = visibleStart + row;
        if (entryIdx < entries.size()) {
            const auto& entry = entries[entryIdx];

            // Format timestamp
            auto time = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::tm tm = *std::localtime(&time);
            char timeBuf[16];
            std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tm);

            // Format level
            std::string levelStr(spdlog::level::to_string_view(entry.level).data());
            std::string color = TerminalRenderer::colorForLevel(entry.level);
            std::string reset = TerminalRenderer::resetColor();

            // Truncate message to fit terminal width (UTF-8 aware)
            // Prefix: [HH:MM:SS] [LEVEL] [loggerName] = 12 + level + loggerName + 6
            size_t prefixLen = 12 + levelStr.size() + utf8::displayWidth(entry.loggerName) + 6;
            size_t maxMsgLen = (termSize.cols > prefixLen) ? termSize.cols - prefixLen : 20;
            std::string msg = entry.message;
            // Remove trailing newlines
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
                msg.pop_back();
            }
            msg = utf8::truncateWithEllipsis(msg, maxMsgLen);

            std::ostringstream line;
            line << "[" << timeBuf << "] "
                 << color << "[" << std::setw(5) << std::left << levelStr << "]" << reset
                 << " [" << entry.loggerName << "] "
                 << msg;

            TerminalRenderer::rawWrite(line.str());
        }
    }
}

void TerminalUI::renderStatusBar(uint16_t row) {
    auto termSize = TerminalRenderer::getTerminalSize();
    TerminalRenderer::moveCursor(row, 1);

    // Status bar with filter info and scroll position
    std::string filterStr;
    switch (_filterLevel) {
        case FilterLevel::All:   filterStr = "ALL"; break;
        case FilterLevel::Info:  filterStr = "INFO+"; break;
        case FilterLevel::Warn:  filterStr = "WARN+"; break;
        case FilterLevel::Error: filterStr = "ERROR"; break;
    }

    auto spdlogLevel = filterToSpdlogLevel(_filterLevel);
    size_t totalLogs = _logBuffer->filteredSize(spdlogLevel);

    std::string scrollStatus = _autoScroll ? "AUTO" : std::to_string(_scrollOffset);

    std::ostringstream status;
    status << TerminalRenderer::reverseVideo()
           << " [1-4]Filter:" << std::setw(6) << std::left << filterStr
           << " Logs:" << std::setw(5) << totalLogs
           << " Scroll:" << std::setw(5) << scrollStatus;

    if (_mode == Mode::SplitScreen) {
        status << " [zoom]Fullscreen [Up/Down]Scroll [PgUp/Dn]Page";
    } else {
        status << " [ESC]Exit Zoom [Up/Down]Scroll [PgUp/Dn]Page";
    }

    // Pad to fill line (UTF-8 aware, ignoring ANSI codes)
    std::string statusStr = status.str();
    size_t visibleLen = utf8::displayWidthIgnoringAnsi(statusStr);
    if (visibleLen < termSize.cols) {
        statusStr += std::string(termSize.cols - visibleLen, ' ');
    }
    statusStr += TerminalRenderer::resetColor();

    TerminalRenderer::rawWrite(statusStr);
}

void TerminalUI::renderCommandPane(uint16_t startRow, uint16_t height) {
    auto termSize = TerminalRenderer::getTerminalSize();

    std::vector<std::string> output;
    {
        std::lock_guard<std::mutex> lock(_outputMutex);
        output = _commandOutput;
    }

    // Reserve last line for prompt
    uint16_t outputHeight = height - 1;

    // Render output lines (oldest to newest, bottom-aligned)
    size_t startIdx = 0;
    if (output.size() > outputHeight) {
        startIdx = output.size() - outputHeight;
    }

    for (uint16_t row = 0; row < outputHeight; ++row) {
        TerminalRenderer::moveCursor(startRow + row, 1);
        TerminalRenderer::clearLine();

        size_t idx = startIdx + row;
        if (idx < output.size()) {
            std::string line = utf8::truncateWithEllipsis(output[idx], termSize.cols);
            TerminalRenderer::rawWrite(line);
        }
    }

    // Render prompt with current input on last line
    uint16_t promptRow = startRow + height - 1;
    TerminalRenderer::moveCursor(promptRow, 1);
    TerminalRenderer::clearLine();

    std::string prompt = "rtype> ";
    std::string inputLine;
    {
        std::lock_guard<std::mutex> lock(_inputMutex);
        inputLine = _inputBuffer;
    }

    std::string fullPrompt = prompt + inputLine;
    size_t promptWidth = utf8::displayWidth(prompt);
    size_t inputWidth = utf8::displayWidth(inputLine);
    size_t fullWidth = promptWidth + inputWidth;

    if (fullWidth > termSize.cols) {
        // Show end of input if too long (UTF-8 aware)
        size_t maxInput = termSize.cols - promptWidth - 3;
        fullPrompt = prompt + "..." + utf8::lastColumns(inputLine, maxInput);
        fullWidth = promptWidth + 3 + utf8::displayWidth(utf8::lastColumns(inputLine, maxInput));
    }

    TerminalRenderer::rawWrite(fullPrompt);

    // Show cursor at end of input (UTF-8 aware column position)
    TerminalRenderer::showCursor();
    TerminalRenderer::moveCursor(promptRow, static_cast<uint16_t>(fullWidth + 1));
}

} // namespace infrastructure::tui
