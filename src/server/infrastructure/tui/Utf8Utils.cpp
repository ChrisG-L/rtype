/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Utf8Utils - UTF-8 string utilities implementation
** Portable wcwidth based on Unicode 15.0 data
*/

#include "infrastructure/tui/Utf8Utils.hpp"
#include <algorithm>

namespace infrastructure::tui::utf8 {

// Unicode combining character ranges (width = 0)
// Based on Unicode 15.0 - Combining Diacritical Marks and similar
static bool isCombining(char32_t cp) {
    // Combining Diacritical Marks
    if (cp >= 0x0300 && cp <= 0x036F) return true;
    // Combining Diacritical Marks Extended
    if (cp >= 0x1AB0 && cp <= 0x1AFF) return true;
    // Combining Diacritical Marks Supplement
    if (cp >= 0x1DC0 && cp <= 0x1DFF) return true;
    // Combining Diacritical Marks for Symbols
    if (cp >= 0x20D0 && cp <= 0x20FF) return true;
    // Combining Half Marks
    if (cp >= 0xFE20 && cp <= 0xFE2F) return true;
    // Zero Width characters
    if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0xFEFF) return true;
    // Variation Selectors
    if (cp >= 0xFE00 && cp <= 0xFE0F) return true;
    if (cp >= 0xE0100 && cp <= 0xE01EF) return true;
    return false;
}

// Unicode wide character ranges (width = 2)
// CJK, Hangul, Emoji, Fullwidth forms
static bool isWide(char32_t cp) {
    // CJK Unified Ideographs
    if (cp >= 0x4E00 && cp <= 0x9FFF) return true;
    // CJK Unified Ideographs Extension A
    if (cp >= 0x3400 && cp <= 0x4DBF) return true;
    // CJK Unified Ideographs Extension B-I
    if (cp >= 0x20000 && cp <= 0x323AF) return true;
    // CJK Compatibility Ideographs
    if (cp >= 0xF900 && cp <= 0xFAFF) return true;
    // Hangul Syllables
    if (cp >= 0xAC00 && cp <= 0xD7AF) return true;
    // Hangul Jamo
    if (cp >= 0x1100 && cp <= 0x11FF) return true;
    // Hangul Compatibility Jamo
    if (cp >= 0x3130 && cp <= 0x318F) return true;
    // Hangul Jamo Extended-A/B
    if (cp >= 0xA960 && cp <= 0xA97F) return true;
    if (cp >= 0xD7B0 && cp <= 0xD7FF) return true;
    // Fullwidth ASCII and Fullwidth punctuation
    if (cp >= 0xFF01 && cp <= 0xFF60) return true;
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return true;
    // CJK Symbols and Punctuation
    if (cp >= 0x3000 && cp <= 0x303F) return true;
    // Hiragana
    if (cp >= 0x3040 && cp <= 0x309F) return true;
    // Katakana
    if (cp >= 0x30A0 && cp <= 0x30FF) return true;
    // Katakana Phonetic Extensions
    if (cp >= 0x31F0 && cp <= 0x31FF) return true;
    // Bopomofo
    if (cp >= 0x3100 && cp <= 0x312F) return true;
    if (cp >= 0x31A0 && cp <= 0x31BF) return true;
    // Enclosed CJK Letters and Months
    if (cp >= 0x3200 && cp <= 0x32FF) return true;
    // CJK Compatibility
    if (cp >= 0x3300 && cp <= 0x33FF) return true;
    // Emoji (most are wide)
    if (cp >= 0x1F300 && cp <= 0x1F9FF) return true;  // Misc Symbols and Pictographs, Emoticons, etc.
    if (cp >= 0x1FA00 && cp <= 0x1FAFF) return true;  // Chess, Extended-A
    if (cp >= 0x2600 && cp <= 0x26FF) return true;    // Misc symbols
    if (cp >= 0x2700 && cp <= 0x27BF) return true;    // Dingbats
    // Regional Indicator Symbols (flags)
    if (cp >= 0x1F1E0 && cp <= 0x1F1FF) return true;
    return false;
}

int charWidth(char32_t codepoint) {
    // Control characters
    if (codepoint < 32 || (codepoint >= 0x7F && codepoint < 0xA0))
        return -1;

    // Soft hyphen
    if (codepoint == 0x00AD)
        return 1;

    // Combining characters have zero width
    if (isCombining(codepoint))
        return 0;

    // Wide characters (CJK, emoji, etc.)
    if (isWide(codepoint))
        return 2;

    // Default: normal width
    return 1;
}

char32_t decodeUtf8Char(const std::string& str, size_t& pos) {
    if (pos >= str.size())
        return 0;

    uint8_t c = static_cast<uint8_t>(str[pos]);

    // ASCII (0xxxxxxx)
    if ((c & 0x80) == 0) {
        pos++;
        return c;
    }

    char32_t codepoint = 0;
    size_t extraBytes = 0;

    // 2-byte sequence (110xxxxx)
    if ((c & 0xE0) == 0xC0) {
        codepoint = c & 0x1F;
        extraBytes = 1;
    }
    // 3-byte sequence (1110xxxx)
    else if ((c & 0xF0) == 0xE0) {
        codepoint = c & 0x0F;
        extraBytes = 2;
    }
    // 4-byte sequence (11110xxx)
    else if ((c & 0xF8) == 0xF0) {
        codepoint = c & 0x07;
        extraBytes = 3;
    }
    // Invalid UTF-8 start byte
    else {
        pos++;
        return 0xFFFD;  // Replacement character
    }

    // Check if we have enough bytes
    if (pos + extraBytes >= str.size()) {
        pos = str.size();
        return 0xFFFD;
    }

    // Read continuation bytes (10xxxxxx)
    for (size_t i = 0; i < extraBytes; ++i) {
        uint8_t next = static_cast<uint8_t>(str[pos + 1 + i]);
        if ((next & 0xC0) != 0x80) {
            // Invalid continuation byte
            pos++;
            return 0xFFFD;
        }
        codepoint = (codepoint << 6) | (next & 0x3F);
    }

    pos += 1 + extraBytes;
    return codepoint;
}

size_t displayWidth(const std::string& str) {
    size_t width = 0;
    size_t pos = 0;

    while (pos < str.size()) {
        char32_t cp = decodeUtf8Char(str, pos);
        int w = charWidth(cp);
        if (w > 0) {
            width += static_cast<size_t>(w);
        }
    }

    return width;
}

std::string truncateToWidth(const std::string& str, size_t maxCols) {
    if (maxCols == 0)
        return "";

    size_t width = 0;
    size_t pos = 0;
    size_t lastValidPos = 0;

    while (pos < str.size()) {
        size_t startPos = pos;
        char32_t cp = decodeUtf8Char(str, pos);
        int w = charWidth(cp);

        if (w < 0) {
            // Skip control characters
            continue;
        }

        if (width + static_cast<size_t>(w) > maxCols) {
            // Would exceed limit
            break;
        }

        width += static_cast<size_t>(w);
        lastValidPos = pos;
    }

    return str.substr(0, lastValidPos);
}

std::string truncateWithEllipsis(const std::string& str, size_t maxCols) {
    if (maxCols < 4) {
        // Not enough space for even "..."
        return truncateToWidth(str, maxCols);
    }

    size_t strWidth = displayWidth(str);
    if (strWidth <= maxCols) {
        return str;
    }

    // Truncate to maxCols - 3 (for "...")
    std::string truncated = truncateToWidth(str, maxCols - 3);
    return truncated + "...";
}

std::string lastColumns(const std::string& str, size_t cols) {
    if (cols == 0)
        return "";

    size_t totalWidth = displayWidth(str);
    if (totalWidth <= cols) {
        return str;
    }

    // We need to skip (totalWidth - cols) columns from the start
    size_t skipCols = totalWidth - cols;
    size_t skipped = 0;
    size_t pos = 0;

    while (pos < str.size() && skipped < skipCols) {
        char32_t cp = decodeUtf8Char(str, pos);
        int w = charWidth(cp);
        if (w > 0) {
            skipped += static_cast<size_t>(w);
        }
    }

    return str.substr(pos);
}

std::string stripAnsiCodes(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    size_t i = 0;
    while (i < str.size()) {
        if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[') {
            // Skip ANSI escape sequence: ESC [ ... m (or other terminator)
            i += 2;
            while (i < str.size() && str[i] != 'm' &&
                   str[i] != 'H' && str[i] != 'J' && str[i] != 'K' &&
                   str[i] != 'A' && str[i] != 'B' && str[i] != 'C' && str[i] != 'D') {
                i++;
            }
            if (i < str.size()) {
                i++;  // Skip the terminator
            }
        } else {
            result += str[i];
            i++;
        }
    }

    return result;
}

size_t displayWidthIgnoringAnsi(const std::string& str) {
    return displayWidth(stripAnsiCodes(str));
}

} // namespace infrastructure::tui::utf8
