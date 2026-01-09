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
    // Clear the callback to prevent use-after-free
    // (LogBuffer may outlive TerminalUI if shared_ptr is held elsewhere)
    if (_logBuffer) {
        _logBuffer->setNewEntryCallback(nullptr);
    }
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
                switch (_mode) {
                    case Mode::SplitScreen:
                        renderSplitScreen();
                        break;
                    case Mode::ZoomLogs:
                        renderZoomMode();
                        break;
                    case Mode::Interact:
                        renderInteractMode();
                        break;
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
    // Delegate to interact mode handler if in interact mode
    if (_mode == Mode::Interact) {
        processInteractKeyInput(ch);
        return;
    }

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

// ============================================================================
// Interact Mode Implementation
// ============================================================================

void TerminalUI::setInteractiveOutput(InteractiveOutput output) {
    std::lock_guard<std::mutex> lock(_interactMutex);
    _interactOutput = std::move(output);
    _interactSelectedIndex = 0;
    _interactScrollOffset = 0;
    _needsRefresh = true;
}

void TerminalUI::enterInteractMode() {
    std::lock_guard<std::mutex> lock(_interactMutex);

    if (_interactOutput.lines.empty()) {
        // Nothing to interact with
        return;
    }

    _previousMode = _mode;
    _mode = Mode::Interact;
    _interactSelectedIndex = 0;
    _interactScrollOffset = 0;
    _needsRefresh = true;
}

void TerminalUI::exitInteractMode() {
    _mode = _previousMode;
    _needsRefresh = true;
}

void TerminalUI::setInteractActionCallback(InteractActionCallback callback) {
    std::lock_guard<std::mutex> lock(_interactMutex);
    _interactCallback = std::move(callback);
}

const SelectableElement* TerminalUI::getSelectedElement() const {
    std::lock_guard<std::mutex> lock(_interactMutex);
    return _interactOutput.getElement(_interactSelectedIndex);
}

void TerminalUI::processInteractKeyInput(int ch) {
    InteractAction pendingAction = InteractAction::None;

    {
        std::lock_guard<std::mutex> lock(_interactMutex);

        switch (ch) {
            case KEY_ESCAPE:
                exitInteractMode();
                break;

            case KEY_LEFT:
                // Navigate to previous element
                if (_interactSelectedIndex > 0) {
                    _interactSelectedIndex--;
                    _needsRefresh = true;
                }
                break;

            case KEY_RIGHT:
                // Navigate to next element
                if (_interactSelectedIndex + 1 < _interactOutput.elements.size()) {
                    _interactSelectedIndex++;
                    _needsRefresh = true;
                }
                break;

            case KEY_UP:
                // Scroll up
                if (_interactScrollOffset > 0) {
                    _interactScrollOffset--;
                    _needsRefresh = true;
                }
                break;

            case KEY_DOWN:
                // Scroll down
                _interactScrollOffset++;
                _needsRefresh = true;
                break;

            case KEY_PAGE_UP: {
                auto termSize = TerminalRenderer::getTerminalSize();
                size_t pageSize = termSize.rows / 2;
                if (_interactScrollOffset >= pageSize) {
                    _interactScrollOffset -= pageSize;
                } else {
                    _interactScrollOffset = 0;
                }
                _needsRefresh = true;
                break;
            }

            case KEY_PAGE_DOWN: {
                auto termSize = TerminalRenderer::getTerminalSize();
                _interactScrollOffset += termSize.rows / 2;
                _needsRefresh = true;
                break;
            }

            case 'b':
            case 'B':
                pendingAction = InteractAction::Ban;
                break;

            case 'k':
            case 'K':
                pendingAction = InteractAction::Kick;
                break;

            case 'c':
            case 'C':
                pendingAction = InteractAction::Copy;
                break;

            case 'u':
            case 'U':
                pendingAction = InteractAction::Unban;
                break;

            case 'x':
            case 'X':
                pendingAction = InteractAction::Close;
                break;

            case 'd':
            case 'D':
                pendingAction = InteractAction::Details;
                break;

            case '\n':
            case '\r':
                pendingAction = InteractAction::Insert;
                break;
        }
    }

    // Execute action outside the lock to avoid deadlock when callback modifies state
    if (pendingAction != InteractAction::None) {
        executeInteractAction(pendingAction);
    }
}

void TerminalUI::executeInteractAction(InteractAction action) {
    SelectableElement elementCopy;
    InteractActionCallback callbackCopy;
    bool valid = false;
    bool shouldExit = false;

    {
        std::lock_guard<std::mutex> lock(_interactMutex);

        const auto* element = _interactOutput.getElement(_interactSelectedIndex);
        if (!element) return;

        // Copy element and callback for use outside lock
        elementCopy = *element;
        callbackCopy = _interactCallback;

        // Check if action is valid for this element type
        switch (action) {
            case InteractAction::Ban:
                valid = (elementCopy.type == ElementType::Email);
                break;
            case InteractAction::Kick:
                // Kick by email - works for any Email element or element with associatedEmail
                valid = (elementCopy.type == ElementType::Email ||
                         elementCopy.associatedEmail.has_value());
                break;
            case InteractAction::Unban:
                valid = (elementCopy.type == ElementType::Email);
                break;
            case InteractAction::Close:
            case InteractAction::Details:
                valid = (elementCopy.type == ElementType::RoomCode);
                break;
            case InteractAction::Copy:
            case InteractAction::Insert:
                valid = true;  // Always available
                break;
            default:
                break;
        }

        if (!valid) return;

        // Determine if we should exit after action
        shouldExit = (action == InteractAction::Ban || action == InteractAction::Kick ||
                      action == InteractAction::Unban || action == InteractAction::Close);
    }

    // Execute callback outside the lock to avoid deadlock
    if (callbackCopy) {
        callbackCopy(action, elementCopy);
    }

    // Exit interact mode after Ban/Kick/Unban/Close actions
    if (shouldExit) {
        std::lock_guard<std::mutex> lock(_interactMutex);
        exitInteractMode();
    }
}

void TerminalUI::renderInteractMode() {
    auto termSize = TerminalRenderer::getTerminalSize();

    // Copy data under lock to avoid holding lock during rendering
    InteractiveOutput outputCopy;
    size_t selectedIdx;
    size_t scrollOffset;
    {
        std::lock_guard<std::mutex> lock(_interactMutex);
        outputCopy = _interactOutput;
        selectedIdx = _interactSelectedIndex;
        scrollOffset = _interactScrollOffset;
    }

    TerminalRenderer::hideCursor();

    // Calculate layout: full screen with status bar at bottom
    uint16_t contentHeight = termSize.rows - 1;  // -1 for status bar

    // Clamp scroll offset
    if (outputCopy.lines.size() > contentHeight) {
        size_t maxScroll = outputCopy.lines.size() - contentHeight;
        scrollOffset = std::min(scrollOffset, maxScroll);
    } else {
        scrollOffset = 0;
    }

    // Get selected element for highlighting
    const auto* selectedElement = outputCopy.getElement(selectedIdx);

    // Render output lines
    for (uint16_t row = 0; row < contentHeight; ++row) {
        TerminalRenderer::moveCursor(row + 1, 1);
        TerminalRenderer::clearLine();

        size_t lineIdx = scrollOffset + row;
        if (lineIdx >= outputCopy.lines.size()) continue;

        const std::string& line = outputCopy.lines[lineIdx];

        // Check if this line contains the selected element
        if (selectedElement && selectedElement->lineIndex == lineIdx) {
            // Render line with highlighted element
            renderLineWithSelection(line, *selectedElement, termSize.cols);
        } else {
            // Render normal line
            std::string truncated = utf8::truncateWithEllipsis(line, termSize.cols);
            TerminalRenderer::rawWrite(truncated);
        }
    }

    // Render status bar (pass the copied data)
    renderInteractStatusBar(termSize.rows, outputCopy, selectedIdx);
}

void TerminalUI::renderInteractStatusBar(uint16_t row,
                                          const InteractiveOutput& output,
                                          size_t selectedIdx) {
    auto termSize = TerminalRenderer::getTerminalSize();
    TerminalRenderer::moveCursor(row, 1);

    const auto* selected = output.getElement(selectedIdx);

    std::ostringstream status;
    status << TerminalRenderer::reverseVideo()
           << " [←→]Select [↑↓]Scroll";

    // Show available actions based on selected element type
    if (selected) {
        switch (selected->type) {
            case ElementType::Email:
                if (output.sourceCommand == "bans") {
                    status << " [U]Unban";
                } else {
                    status << " [B]Ban [K]Kick";
                }
                break;
            case ElementType::PlayerId:
                status << " [K]Kick";
                break;
            case ElementType::RoomCode:
                status << " [D]Details [X]Close";
                break;
            default:
                break;
        }
        status << " [C]Copy [Enter]Insert";
    }

    status << " [ESC]Exit";

    // Show selection info
    if (selected) {
        status << "  │ " << utf8::truncateWithEllipsis(selected->value, 30);
    }

    // Pad to fill line
    std::string statusStr = status.str();
    size_t visibleLen = utf8::displayWidthIgnoringAnsi(statusStr);
    if (visibleLen < termSize.cols) {
        statusStr += std::string(termSize.cols - visibleLen, ' ');
    }
    statusStr += TerminalRenderer::resetColor();

    TerminalRenderer::rawWrite(statusStr);
}

void TerminalUI::renderLineWithSelection(const std::string& line,
                                          const SelectableElement& element,
                                          uint16_t maxCols) {
    // We need to render the line with the selected element highlighted
    // The element has startCol and endCol in display width units

    size_t currentCol = 0;
    size_t bytePos = 0;
    std::string output;

    // Render characters before selection
    while (bytePos < line.size() && currentCol < element.startCol) {
        size_t nextPos = bytePos;
        char32_t cp = utf8::decodeUtf8Char(line, nextPos);
        if (nextPos == bytePos) break;  // Invalid UTF-8

        int charWidth = utf8::charWidth(cp);
        if (currentCol + charWidth > element.startCol) break;

        output += line.substr(bytePos, nextPos - bytePos);
        currentCol += charWidth;
        bytePos = nextPos;
    }

    // Start highlight (reverse video)
    output += TerminalRenderer::reverseVideo();

    // Render the full value of the element (not truncated)
    output += element.value;
    currentCol += utf8::displayWidth(element.value);

    // End highlight
    output += TerminalRenderer::resetColor();

    // Skip past the original truncated element in the source line
    while (bytePos < line.size() && currentCol < element.endCol) {
        size_t nextPos = bytePos;
        char32_t cp = utf8::decodeUtf8Char(line, nextPos);
        if (nextPos == bytePos) break;

        int charWidth = utf8::charWidth(cp);
        bytePos = nextPos;
        // Don't add to currentCol here, we already added the full value
    }

    // Skip remaining characters of the original element
    size_t originalEndCol = element.endCol;
    while (bytePos < line.size()) {
        size_t nextPos = bytePos;
        char32_t cp = utf8::decodeUtf8Char(line, nextPos);
        if (nextPos == bytePos) break;

        size_t testCol = 0;
        size_t testPos = 0;
        while (testPos < bytePos) {
            size_t np = testPos;
            char32_t c = utf8::decodeUtf8Char(line, np);
            if (np == testPos) break;
            testCol += utf8::charWidth(c);
            testPos = np;
        }

        if (testCol >= originalEndCol) {
            // We've passed the end of the original element
            // Render the rest of the line
            output += line.substr(bytePos);
            break;
        }
        bytePos = nextPos;
    }

    // Truncate to fit terminal
    std::string truncated = utf8::truncateWithEllipsis(output, maxCols);
    TerminalRenderer::rawWrite(truncated);
}

} // namespace infrastructure::tui
