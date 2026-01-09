/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TerminalRenderer implementation - Cross-platform ANSI terminal control
*/

#include "infrastructure/tui/TerminalRenderer.hpp"
#include <iostream>
#include <cstdio>

#ifdef _WIN32
    #include <io.h>
    // Note: We use ReadConsoleInput instead of _kbhit()/_getch() from conio.h
    // because ReadConsoleInput gives us virtual key codes (VK_UP, VK_DOWN, etc.)
    // directly, which is more reliable than parsing scan codes or VT sequences.
#else
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <poll.h>
#endif

namespace infrastructure::tui {

// Static member initialization
bool TerminalRenderer::s_rawModeEnabled = false;

#ifdef _WIN32
DWORD TerminalRenderer::s_originalInputMode = 0;
DWORD TerminalRenderer::s_originalOutputMode = 0;
HANDLE TerminalRenderer::s_hConsoleIn = INVALID_HANDLE_VALUE;
HANDLE TerminalRenderer::s_hConsoleOut = INVALID_HANDLE_VALUE;
#else
struct termios TerminalRenderer::s_originalTermios;
#endif

TerminalRenderer::Size TerminalRenderer::getTerminalSize() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return {
            static_cast<uint16_t>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1),
            static_cast<uint16_t>(csbi.srWindow.Right - csbi.srWindow.Left + 1)
        };
    }
    return {24, 80};  // Default fallback
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return {static_cast<uint16_t>(w.ws_row), static_cast<uint16_t>(w.ws_col)};
    }
    return {24, 80};  // Default fallback
#endif
}

void TerminalRenderer::moveCursor(uint16_t row, uint16_t col) {
    // ANSI: ESC[row;colH (1-based)
    rawWrite("\033[" + std::to_string(row) + ";" + std::to_string(col) + "H");
}

void TerminalRenderer::saveCursor() {
    rawWrite("\033[s");
}

void TerminalRenderer::restoreCursor() {
    rawWrite("\033[u");
}

void TerminalRenderer::hideCursor() {
    rawWrite("\033[?25l");
}

void TerminalRenderer::showCursor() {
    rawWrite("\033[?25h");
}

void TerminalRenderer::clearScreen() {
    rawWrite("\033[2J");  // Clear entire screen
    moveCursor(1, 1);     // Move to top-left
}

void TerminalRenderer::clearLine() {
    rawWrite("\033[2K");  // Clear entire line
}

void TerminalRenderer::clearToEndOfLine() {
    rawWrite("\033[K");
}

void TerminalRenderer::clearFromCursor() {
    rawWrite("\033[J");  // Clear from cursor to end of screen
}

void TerminalRenderer::scrollRegion(uint16_t top, uint16_t bottom) {
    // ANSI: ESC[top;bottomr - Set scroll region
    rawWrite("\033[" + std::to_string(top) + ";" + std::to_string(bottom) + "r");
}

void TerminalRenderer::resetScrollRegion() {
    rawWrite("\033[r");  // Reset to full screen
}

std::string TerminalRenderer::colorForLevel(spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::trace:    return "\033[90m";      // Dark gray
        case spdlog::level::debug:    return "\033[36m";      // Cyan
        case spdlog::level::info:     return "\033[32m";      // Green
        case spdlog::level::warn:     return "\033[33m";      // Yellow
        case spdlog::level::err:      return "\033[31m";      // Red
        case spdlog::level::critical: return "\033[1;31m";    // Bold red
        default:                      return "\033[0m";
    }
}

std::string TerminalRenderer::resetColor() {
    return "\033[0m";
}

std::string TerminalRenderer::boldText() {
    return "\033[1m";
}

std::string TerminalRenderer::reverseVideo() {
    return "\033[7m";
}

void TerminalRenderer::drawHorizontalLine(uint16_t row, uint16_t startCol, uint16_t width) {
    moveCursor(row, startCol);
    rawWrite(std::string(width, '-'));
}

void TerminalRenderer::rawWrite(const std::string& str) {
#ifdef _WIN32
    DWORD written;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), str.c_str(),
                  static_cast<DWORD>(str.size()), &written, nullptr);
#else
    [[maybe_unused]] auto result = write(STDOUT_FILENO, str.c_str(), str.size());
#endif
}

void TerminalRenderer::flush() {
    std::fflush(stdout);
}

void TerminalRenderer::enableRawMode() {
    if (s_rawModeEnabled) return;

#ifdef _WIN32
    s_hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
    s_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // Save original modes
    GetConsoleMode(s_hConsoleIn, &s_originalInputMode);
    GetConsoleMode(s_hConsoleOut, &s_originalOutputMode);

    // Configure input mode: disable line buffering and echo
    // Note: We do NOT enable ENABLE_VIRTUAL_TERMINAL_INPUT because we use
    // ReadConsoleInput() which gives us virtual key codes directly, not VT sequences.
    DWORD inputMode = s_originalInputMode;
    inputMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(s_hConsoleIn, inputMode);

    // Enable ANSI escape sequences for output
    DWORD outputMode = s_originalOutputMode;
    outputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(s_hConsoleOut, outputMode);
#else
    tcgetattr(STDIN_FILENO, &s_originalTermios);
    struct termios raw = s_originalTermios;
    raw.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
    raw.c_cc[VMIN] = 0;   // Non-blocking read
    raw.c_cc[VTIME] = 0;  // No timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif

    s_rawModeEnabled = true;
}

void TerminalRenderer::disableRawMode() {
    if (!s_rawModeEnabled) return;

#ifdef _WIN32
    SetConsoleMode(s_hConsoleIn, s_originalInputMode);
    SetConsoleMode(s_hConsoleOut, s_originalOutputMode);
#else
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &s_originalTermios);
#endif

    s_rawModeEnabled = false;
}

bool TerminalRenderer::isRawModeEnabled() {
    return s_rawModeEnabled;
}

void TerminalRenderer::enableAlternateBuffer() {
    rawWrite("\033[?1049h");  // Switch to alternate screen buffer
}

void TerminalRenderer::disableAlternateBuffer() {
    rawWrite("\033[?1049l");  // Switch back to main screen buffer
}

int TerminalRenderer::readChar() {
#ifdef _WIN32
    // Use ReadConsoleInput for proper handling of all key events
    // This gives us virtual key codes directly (VK_UP, VK_DOWN, etc.)
    INPUT_RECORD inputRecord;
    DWORD eventsRead;

    // Check if input is available (non-blocking)
    DWORD eventsAvailable = 0;
    if (!GetNumberOfConsoleInputEvents(s_hConsoleIn, &eventsAvailable) || eventsAvailable == 0) {
        return -1;
    }

    // Peek to check for key events (we need to skip non-key events)
    while (eventsAvailable > 0) {
        if (!PeekConsoleInput(s_hConsoleIn, &inputRecord, 1, &eventsRead) || eventsRead == 0) {
            return -1;
        }

        // Skip non-key events and key-up events
        if (inputRecord.EventType != KEY_EVENT || !inputRecord.Event.KeyEvent.bKeyDown) {
            // Consume and discard this event
            ReadConsoleInput(s_hConsoleIn, &inputRecord, 1, &eventsRead);
            GetNumberOfConsoleInputEvents(s_hConsoleIn, &eventsAvailable);
            continue;
        }

        // Consume the key event
        ReadConsoleInput(s_hConsoleIn, &inputRecord, 1, &eventsRead);

        // Handle virtual key codes for special keys
        WORD vk = inputRecord.Event.KeyEvent.wVirtualKeyCode;
        switch (vk) {
            case VK_UP:     return 0x1001;  // Up arrow
            case VK_DOWN:   return 0x1002;  // Down arrow
            case VK_LEFT:   return 0x1003;  // Left arrow
            case VK_RIGHT:  return 0x1004;  // Right arrow
            case VK_PRIOR:  return 0x1005;  // Page Up
            case VK_NEXT:   return 0x1006;  // Page Down
            case VK_ESCAPE: return 27;      // Escape
            case VK_RETURN: return '\r';    // Enter
            case VK_BACK:   return '\b';    // Backspace
        }

        // For regular characters, use the translated character
        char ch = inputRecord.Event.KeyEvent.uChar.AsciiChar;
        if (ch != 0) {
            return ch;
        }

        // Unknown key, try next event
        GetNumberOfConsoleInputEvents(s_hConsoleIn, &eventsAvailable);
    }
    return -1;
#else
    // Check if input is available
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    if (poll(&pfd, 1, 0) <= 0) {
        return -1;
    }

    char ch;
    if (read(STDIN_FILENO, &ch, 1) != 1) {
        return -1;
    }

    // Handle escape sequences
    if (ch == '\033') {
        char seq[3];
        // Read with small timeout for escape sequence
        struct pollfd pfd2;
        pfd2.fd = STDIN_FILENO;
        pfd2.events = POLLIN;
        if (poll(&pfd2, 1, 10) <= 0) {
            return '\033';  // Just escape key
        }
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\033';
        if (seq[0] != '[') return '\033';

        if (poll(&pfd2, 1, 10) <= 0) return '\033';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\033';

        switch (seq[1]) {
            case 'A': return 0x1001;  // Up arrow
            case 'B': return 0x1002;  // Down arrow
            case 'C': return 0x1004;  // Right arrow
            case 'D': return 0x1003;  // Left arrow
            case '5':  // Page Up (ESC [ 5 ~)
                if (poll(&pfd2, 1, 10) > 0) read(STDIN_FILENO, &seq[2], 1);
                return 0x1005;
            case '6':  // Page Down (ESC [ 6 ~)
                if (poll(&pfd2, 1, 10) > 0) read(STDIN_FILENO, &seq[2], 1);
                return 0x1006;
            default:
                return '\033';
        }
    }
    return ch;
#endif
}

} // namespace infrastructure::tui
