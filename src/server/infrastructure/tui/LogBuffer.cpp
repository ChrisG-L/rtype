/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** LogBuffer implementation
*/

#include "infrastructure/tui/LogBuffer.hpp"
#include <algorithm>

namespace infrastructure::tui {

LogBuffer::LogBuffer(size_t capacity)
    : _capacity(capacity)
{
    _entries.resize(capacity);
}

void LogBuffer::push(LogEntry entry) {
    NewEntryCallback callback;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _entries[_head] = std::move(entry);
        _head = (_head + 1) % _capacity;
        if (_count < _capacity) {
            ++_count;
        }
        callback = _newEntryCallback;
    }

    // Call callback outside lock to avoid deadlock
    if (callback) {
        callback();
    }
}

std::vector<LogEntry> LogBuffer::getFiltered(spdlog::level::level_enum minLevel) const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<LogEntry> result;
    result.reserve(_count);

    // Start from oldest entry
    size_t start = (_count < _capacity) ? 0 : _head;
    for (size_t i = 0; i < _count; ++i) {
        size_t idx = (start + i) % _capacity;
        if (_entries[idx].level >= minLevel) {
            result.push_back(_entries[idx]);
        }
    }
    return result;
}

std::vector<LogEntry> LogBuffer::getRange(size_t start, size_t count,
                                           spdlog::level::level_enum minLevel) const {
    auto filtered = getFiltered(minLevel);
    if (start >= filtered.size()) {
        return {};
    }
    size_t actualCount = std::min(count, filtered.size() - start);
    return std::vector<LogEntry>(
        filtered.begin() + static_cast<std::ptrdiff_t>(start),
        filtered.begin() + static_cast<std::ptrdiff_t>(start + actualCount)
    );
}

size_t LogBuffer::size() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _count;
}

size_t LogBuffer::filteredSize(spdlog::level::level_enum minLevel) const {
    std::lock_guard<std::mutex> lock(_mutex);
    size_t count = 0;

    size_t start = (_count < _capacity) ? 0 : _head;
    for (size_t i = 0; i < _count; ++i) {
        size_t idx = (start + i) % _capacity;
        if (_entries[idx].level >= minLevel) {
            ++count;
        }
    }
    return count;
}

void LogBuffer::clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    _head = 0;
    _count = 0;
}

void LogBuffer::setNewEntryCallback(NewEntryCallback callback) {
    std::lock_guard<std::mutex> lock(_mutex);
    _newEntryCallback = std::move(callback);
}

} // namespace infrastructure::tui
