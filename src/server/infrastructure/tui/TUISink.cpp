/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TUISink implementation
*/

#include "infrastructure/tui/TUISink.hpp"

namespace infrastructure::tui {

template<typename Mutex>
TUISink<Mutex>::TUISink(std::shared_ptr<LogBuffer> buffer)
    : _buffer(std::move(buffer))
{
}

template<typename Mutex>
void TUISink<Mutex>::sink_it_(const spdlog::details::log_msg& msg) {
    LogEntry entry{
        .timestamp = msg.time,
        .level = msg.level,
        .loggerName = std::string(msg.logger_name.data(), msg.logger_name.size()),
        .message = std::string(msg.payload.data(), msg.payload.size())
    };
    _buffer->push(std::move(entry));
}

template<typename Mutex>
void TUISink<Mutex>::flush_() {
    // No-op for memory buffer - data is immediately available
}

// Explicit template instantiations
template class TUISink<std::mutex>;
template class TUISink<spdlog::details::null_mutex>;

} // namespace infrastructure::tui
