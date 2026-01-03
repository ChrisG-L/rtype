/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** InteractiveOutput - Data structures for interact mode
*/

#ifndef INTERACTIVE_OUTPUT_HPP_
#define INTERACTIVE_OUTPUT_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <optional>
#include <functional>

namespace infrastructure::tui {

/**
 * @brief Type of selectable element for determining available actions
 */
enum class ElementType {
    Email,          // Can: Ban, Kick (via lookup), Copy
    DisplayName,    // Can: Copy
    PlayerId,       // Can: Kick, Copy
    Endpoint,       // Can: Copy
    Generic         // Can: Copy only
};

/**
 * @brief A selectable element within interactive output
 *
 * Each element represents a cell in a table that can be selected
 * and acted upon (Ban, Kick, Copy, etc.)
 */
struct SelectableElement {
    size_t lineIndex;           ///< Line index in output (0-based)
    size_t startCol;            ///< Start column (display width, 0-based)
    size_t endCol;              ///< End column (display width, exclusive)
    std::string value;          ///< Full value (non-truncated)
    std::string truncatedValue; ///< Value as displayed (truncated)
    ElementType type;           ///< Type for determining actions

    // Optional context for actions (e.g., player ID for email rows)
    std::optional<uint8_t> associatedPlayerId;
    std::optional<std::string> associatedEmail;
};

/**
 * @brief Interactive output from a command
 *
 * Stores the rendered output lines along with metadata about
 * selectable elements for navigation in interact mode.
 */
struct InteractiveOutput {
    std::vector<std::string> lines;             ///< Rendered output lines
    std::vector<SelectableElement> elements;    ///< Selectable elements
    std::string sourceCommand;                  ///< Command that generated this output

    /**
     * @brief Check if this output has any selectable elements
     */
    bool hasSelectables() const { return !elements.empty(); }

    /**
     * @brief Get element at given index (with bounds check)
     */
    const SelectableElement* getElement(size_t index) const {
        return index < elements.size() ? &elements[index] : nullptr;
    }

    /**
     * @brief Find next element index in navigation direction
     * @param currentIndex Current selection (-1 for none)
     * @param forward True for right/next, false for left/previous
     * @return New index or currentIndex if at boundary
     */
    size_t navigate(size_t currentIndex, bool forward) const {
        if (elements.empty()) return 0;
        if (forward) {
            return (currentIndex + 1) < elements.size() ? currentIndex + 1 : currentIndex;
        } else {
            return currentIndex > 0 ? currentIndex - 1 : 0;
        }
    }
};

/**
 * @brief Actions available in interact mode
 */
enum class InteractAction {
    None,
    Ban,        // B key - Ban selected email
    Kick,       // K key - Kick player
    Copy,       // C key - Copy to clipboard
    Unban,      // U key - Unban (for bans list)
    Insert,     // Enter - Insert value into prompt
    Exit        // Escape - Exit interact mode
};

/**
 * @brief Callback type for executing interact actions
 */
using InteractActionCallback = std::function<void(InteractAction, const SelectableElement&)>;

} // namespace infrastructure::tui

#endif /* !INTERACTIVE_OUTPUT_HPP_ */
