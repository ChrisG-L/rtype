/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Utf8Utils - UTF-8 string utilities for terminal display
*/

#ifndef UTF8_UTILS_HPP_
#define UTF8_UTILS_HPP_

#include <string>
#include <cstdint>

namespace infrastructure::tui::utf8 {

/**
 * @brief Get the display width of a Unicode codepoint
 * @param codepoint The Unicode codepoint (U+0000 to U+10FFFF)
 * @return -1 for control chars, 0 for combining marks, 1 for normal, 2 for wide (CJK/emoji)
 */
int charWidth(char32_t codepoint);

/**
 * @brief Decode a UTF-8 character from a string
 * @param str The string to decode from
 * @param pos Current position (will be advanced past the character)
 * @return The decoded codepoint, or 0xFFFD (replacement char) on error
 */
char32_t decodeUtf8Char(const std::string& str, size_t& pos);

/**
 * @brief Calculate the display width (in terminal columns) of a UTF-8 string
 * @param str The UTF-8 encoded string
 * @return The number of terminal columns the string would occupy
 */
size_t displayWidth(const std::string& str);

/**
 * @brief Truncate a UTF-8 string to fit within a maximum column width
 * @param str The UTF-8 encoded string
 * @param maxCols Maximum number of terminal columns
 * @return Truncated string that fits within maxCols (never splits UTF-8 chars)
 */
std::string truncateToWidth(const std::string& str, size_t maxCols);

/**
 * @brief Truncate a UTF-8 string with ellipsis ("...") if it exceeds maxCols
 * @param str The UTF-8 encoded string
 * @param maxCols Maximum number of terminal columns (including ellipsis if needed)
 * @return Truncated string with "..." appended if truncation occurred
 */
std::string truncateWithEllipsis(const std::string& str, size_t maxCols);

/**
 * @brief Extract the last N columns from a UTF-8 string
 * @param str The UTF-8 encoded string
 * @param cols Number of columns to extract from the end
 * @return The rightmost portion of the string fitting within cols
 */
std::string lastColumns(const std::string& str, size_t cols);

/**
 * @brief Strip ANSI escape codes from a string
 * @param str String potentially containing ANSI codes
 * @return String with all ANSI escape sequences removed
 */
std::string stripAnsiCodes(const std::string& str);

/**
 * @brief Calculate display width of a string, ignoring ANSI codes
 * @param str String potentially containing ANSI codes
 * @return Display width excluding ANSI escape sequences
 */
size_t displayWidthIgnoringAnsi(const std::string& str);

} // namespace infrastructure::tui::utf8

#endif /* !UTF8_UTILS_HPP_ */
