/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** LogBuffer - Thread-safe circular buffer for log entries
*/

#ifndef LOG_BUFFER_HPP_
#define LOG_BUFFER_HPP_

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <spdlog/spdlog.h>

namespace infrastructure::tui {

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    spdlog::level::level_enum level;
    std::string loggerName;
    std::string message;
};

class LogBuffer {
public:
    static constexpr size_t DEFAULT_CAPACITY = 1000;

    explicit LogBuffer(size_t capacity = DEFAULT_CAPACITY);
    ~LogBuffer() = default;

    // Thread-safe operations
    void push(LogEntry entry);
    std::vector<LogEntry> getFiltered(spdlog::level::level_enum minLevel) const;
    std::vector<LogEntry> getRange(size_t start, size_t count,
                                    spdlog::level::level_enum minLevel) const;
    size_t size() const;
    size_t filteredSize(spdlog::level::level_enum minLevel) const;
    void clear();

    // Notification callback for new entries
    using NewEntryCallback = std::function<void()>;
    void setNewEntryCallback(NewEntryCallback callback);

private:
    mutable std::mutex _mutex;
    std::vector<LogEntry> _entries;
    size_t _capacity;
    size_t _head = 0;   // Index for next write
    size_t _count = 0;  // Current number of entries
    NewEntryCallback _newEntryCallback;
};

} // namespace infrastructure::tui

#endif /* !LOG_BUFFER_HPP_ */
