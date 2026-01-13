/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TerminalUI - Main TUI controller for split-screen log display
*/

#ifndef TERMINAL_UI_HPP_
#define TERMINAL_UI_HPP_

#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <spdlog/spdlog.h>
#include "LogBuffer.hpp"
#include "TerminalRenderer.hpp"
#include "InteractiveOutput.hpp"

namespace infrastructure::tui {

// Special key codes returned by TerminalRenderer::readChar()
constexpr int KEY_UP = 0x1001;
constexpr int KEY_DOWN = 0x1002;
constexpr int KEY_LEFT = 0x1003;
constexpr int KEY_RIGHT = 0x1004;
constexpr int KEY_PAGE_UP = 0x1005;
constexpr int KEY_PAGE_DOWN = 0x1006;
constexpr int KEY_ESCAPE = 27;

class TerminalUI {
public:
    enum class Mode { SplitScreen, ZoomLogs, Interact, NetworkMonitor };
    enum class FilterLevel { All = 0, Info = 1, Warn = 2, Error = 3 };

    explicit TerminalUI(std::shared_ptr<LogBuffer> logBuffer);
    ~TerminalUI();

    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;

    // Mode control
    void setMode(Mode mode);
    Mode getMode() const;

    // Log filtering (keyboard shortcuts 1-4)
    void setFilter(FilterLevel level);
    FilterLevel getFilter() const;

    // Scrolling
    void scrollUp(size_t lines = 1);
    void scrollDown(size_t lines = 1);
    void pageUp();
    void pageDown();
    void scrollToBottom();

    // CLI output (printed to command pane)
    void printToCommandPane(const std::string& text);
    void showPrompt(const std::string& prompt = "rtype> ");

    // Input handling - returns completed command line or empty string
    std::string processInputAndGetCommand();

    // Manual refresh
    void refresh();
    void requestRefresh();

    // Interact mode
    void setInteractiveOutput(InteractiveOutput output);
    void enterInteractMode();
    void exitInteractMode();
    bool isInInteractMode() const { return _mode == Mode::Interact; }

    // Set callback for interact actions (called when user performs an action)
    void setInteractActionCallback(InteractActionCallback callback);

    // Get currently selected element (nullptr if none)
    const SelectableElement* getSelectedElement() const;

    // Edit mode control
    void enterEditMode(const SelectableElement& element);
    void exitEditMode(bool confirm);
    bool isInEditMode() const { return _editModeActive; }

    // Edit mode callback (called when user confirms an edit)
    using EditConfirmCallback = std::function<void(const SelectableElement&, const std::string&)>;
    void setEditConfirmCallback(EditConfirmCallback callback);

    // Network monitor mode
    using NetworkMonitorCallback = std::function<std::string()>;
    void setNetworkMonitorCallback(NetworkMonitorCallback callback);
    void enterNetworkMonitorMode();
    void exitNetworkMonitorMode();
    bool isInNetworkMonitorMode() const { return _mode == Mode::NetworkMonitor; }
    void setNetworkRefreshInterval(std::chrono::milliseconds interval);
    std::chrono::milliseconds getNetworkRefreshInterval() const { return _networkRefreshInterval; }
    void toggleRoomsCollapsed();
    bool areRoomsCollapsed() const { return _roomsCollapsed; }

private:
    void renderLoop();
    void renderSplitScreen();
    void renderZoomMode();
    void renderLogPane(uint16_t startRow, uint16_t height);
    void renderCommandPane(uint16_t startRow, uint16_t height);
    void renderStatusBar(uint16_t row);

    void processKeyInput(int ch);
    void processInteractKeyInput(int ch);
    void executeInteractAction(InteractAction action);
    spdlog::level::level_enum filterToSpdlogLevel(FilterLevel level) const;

    // Interact mode rendering
    void renderInteractMode();
    void renderInteractStatusBar(uint16_t row,
                                  const InteractiveOutput& output,
                                  size_t selectedIdx);
    void renderLineWithSelection(const std::string& line,
                                  const SelectableElement& element,
                                  uint16_t maxCols);

    // Edit mode processing and rendering
    void processEditKeyInput(int ch);
    void renderEditStatusBar(uint16_t row);

    // Network monitor mode processing and rendering
    void processNetworkMonitorKeyInput(int ch);
    void renderNetworkMonitor();
    void renderNetworkMonitorStatusBar(uint16_t row);

    std::shared_ptr<LogBuffer> _logBuffer;
    Mode _mode = Mode::SplitScreen;
    FilterLevel _filterLevel = FilterLevel::All;

    // Scroll state
    size_t _scrollOffset = 0;  // 0 = bottom (newest)
    bool _autoScroll = true;

    // Input buffer
    std::string _inputBuffer;
    std::string _completedCommand;
    mutable std::mutex _inputMutex;

    // Command pane output buffer
    std::vector<std::string> _commandOutput;
    mutable std::mutex _outputMutex;
    static constexpr size_t MAX_COMMAND_OUTPUT = 50;

    // Interact mode state
    InteractiveOutput _interactOutput;
    size_t _interactSelectedIndex = 0;
    size_t _interactScrollOffset = 0;
    Mode _previousMode = Mode::SplitScreen;  // Mode to return to after interact
    InteractActionCallback _interactCallback;
    mutable std::mutex _interactMutex;

    // Edit mode state
    bool _editModeActive = false;
    std::string _editBuffer;           // Value being edited
    std::string _editFieldName;        // Field name (email, username, password)
    size_t _editCursorPos = 0;         // Cursor position in _editBuffer
    SelectableElement _editElement;    // Element being edited
    EditConfirmCallback _editConfirmCallback;

    // Network monitor mode state
    NetworkMonitorCallback _networkMonitorCallback;
    std::chrono::milliseconds _networkRefreshInterval{1000};
    std::chrono::steady_clock::time_point _lastNetworkRefresh;
    bool _roomsCollapsed{false};
    std::string _networkMonitorContent;  // Cached content from callback
    size_t _networkScrollOffset{0};      // Scroll position in network view
    mutable std::mutex _networkMutex;

    // Render thread
    std::jthread _renderThread;
    std::atomic<bool> _running{false};
    std::atomic<bool> _needsRefresh{true};

    // Layout constants
    static constexpr uint16_t LOG_PANE_HEIGHT = 12;
    static constexpr uint16_t STATUS_BAR_HEIGHT = 1;
    static constexpr uint16_t MIN_COMMAND_PANE_HEIGHT = 5;
    static constexpr uint16_t RENDER_INTERVAL_MS = 50;  // 20 FPS
};

} // namespace infrastructure::tui

#endif /* !TERMINAL_UI_HPP_ */
