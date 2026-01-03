/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TUISink - Custom spdlog sink that captures logs to a buffer for TUI display
*/

#ifndef TUI_SINK_HPP_
#define TUI_SINK_HPP_

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#include <memory>
#include <mutex>
#include "LogBuffer.hpp"

namespace infrastructure::tui {

template<typename Mutex>
class TUISink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit TUISink(std::shared_ptr<LogBuffer> buffer);

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override;

private:
    std::shared_ptr<LogBuffer> _buffer;
};

// Thread-safe version (multi-threaded)
using TUISink_mt = TUISink<std::mutex>;

// Single-threaded version
using TUISink_st = TUISink<spdlog::details::null_mutex>;

} // namespace infrastructure::tui

#endif /* !TUI_SINK_HPP_ */
