/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TerminalRenderer - Cross-platform ANSI terminal control
*/

#ifndef TERMINAL_RENDERER_HPP_
#define TERMINAL_RENDERER_HPP_

#include <string>
#include <cstdint>
#include <spdlog/spdlog.h>

#ifdef _WIN32
    // WIN32_LEAN_AND_MEAN prevents windows.h from including winsock.h
    // which would conflict with winsock2.h used by Boost.Asio
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <termios.h>
#endif

namespace infrastructure::tui {

class TerminalRenderer {
public:
    // Terminal dimensions
    struct Size {
        uint16_t rows;
        uint16_t cols;
    };
    static Size getTerminalSize();

    // Cursor control
    static void moveCursor(uint16_t row, uint16_t col);
    static void saveCursor();
    static void restoreCursor();
    static void hideCursor();
    static void showCursor();

    // Screen control
    static void clearScreen();
    static void clearLine();
    static void clearToEndOfLine();
    static void clearFromCursor();
    static void scrollRegion(uint16_t top, uint16_t bottom);
    static void resetScrollRegion();

    // Colors
    static std::string colorForLevel(spdlog::level::level_enum level);
    static std::string resetColor();
    static std::string boldText();
    static std::string reverseVideo();

    // Box drawing
    static void drawHorizontalLine(uint16_t row, uint16_t startCol, uint16_t width);

    // Raw output (bypasses buffering)
    static void rawWrite(const std::string& str);
    static void flush();

    // Terminal mode control
    static void enableRawMode();
    static void disableRawMode();
    static void enableAlternateBuffer();
    static void disableAlternateBuffer();
    static bool isRawModeEnabled();

    // Input reading (non-blocking)
    static int readChar();  // Returns -1 if no input available

private:
    static bool s_rawModeEnabled;
#ifdef _WIN32
    static DWORD s_originalInputMode;
    static DWORD s_originalOutputMode;
    static HANDLE s_hConsoleIn;
    static HANDLE s_hConsoleOut;
#else
    static struct termios s_originalTermios;
#endif
};

} // namespace infrastructure::tui

#endif /* !TERMINAL_RENDERER_HPP_ */
