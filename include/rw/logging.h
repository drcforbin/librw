#ifndef CSXP_LOGGING_H
#define CSXP_LOGGING_H

#include "fmt/format.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <stdlib.h>
#include <string>
#include <string_view>

namespace rw::logging {

enum class log_level
{
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    err = 4,
    fatal = 5,
    off = 6
};

class Logger
{
public:
    Logger(std::string_view name) :
        m_name(name),
        m_level(log_level::trace)
    {}

    virtual ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string_view name() const { return m_name; }

    log_level level() { return m_level; }
    void level(log_level lvl) { m_level = lvl; }

    template <typename... Args>
    void log(log_level lvl, std::string_view fmt, const Args&... args);
    template <typename... Args>
    void log(log_level lvl, std::string_view msg);
    template <typename Arg1, typename... Args>
    void trace(std::string_view fmt, const Arg1&, const Args&... args);
    template <typename Arg1, typename... Args>
    void debug(std::string_view fmt, const Arg1&, const Args&... args);
    template <typename Arg1, typename... Args>
    void info(std::string_view fmt, const Arg1&, const Args&... args);
    template <typename Arg1, typename... Args>
    void warn(std::string_view fmt, const Arg1&, const Args&... args);
    template <typename Arg1, typename... Args>
    void error(std::string_view fmt, const Arg1&, const Args&... args);
    template <typename Arg1, typename... Args>
    void fatal(std::string_view fmt, const Arg1&, const Args&... args);

    template <typename T>
    void log(log_level lvl, const T&);
    template <typename T>
    void trace(const T&);
    template <typename T>
    void debug(const T&);
    template <typename T>
    void info(const T&);
    template <typename T>
    void warn(const T&);
    template <typename T>
    void error(const T&);
    template <typename T>
    void fatal(const T&);

private:
    const std::string m_name;
    log_level m_level;
};

// get/create a logger
std::shared_ptr<Logger> get(std::string_view name);

// get/create a logger for debugging
std::shared_ptr<Logger> dbg();

namespace details {
struct Message
{
    Message() = default;
    Message(std::string_view logname, log_level lvl) :
        logname(logname),
        level(lvl),
        ts(std::chrono::system_clock::now())
    {}

    Message(const Message& other) = delete;
    Message& operator=(Message&& other) = delete;
    Message(Message&& other) = delete;

    std::string_view logname;
    log_level level = log_level::trace;
    std::chrono::system_clock::time_point ts;

    std::string msg;
};

void log_message(const Message& msg);

} // namespace details

// impl bits:

template <typename... Args>
inline void Logger::log(log_level lvl, std::string_view fmt, const Args&... args)
{
    if (lvl < m_level)
        return;

    details::Message message(m_name, lvl);
    message.msg = fmt::format(fmt, args...);
    details::log_message(message);

    if (lvl == log_level::fatal)
        std::exit(EXIT_FAILURE);
}

template <typename... Args>
inline void Logger::log(log_level lvl, std::string_view msg)
{
    if (lvl < m_level)
        return;

    details::Message message(m_name, lvl);
    message.msg = msg;
    details::log_message(message);

    if (lvl == log_level::fatal)
        std::exit(EXIT_FAILURE);
}

template <typename T>
inline void Logger::log(log_level lvl, const T& msg)
{
    if (lvl < m_level)
        return;

    details::Message message(m_name, lvl);
    message.msg = fmt::format(msg);
    details::log_message(message);

    if (lvl == log_level::fatal)
        std::exit(EXIT_FAILURE);
}

template <typename Arg1, typename... Args>
inline void Logger::trace(std::string_view fmt, const Arg1& arg1, const Args&... args)
{
    log(log_level::trace, fmt, arg1, args...);
}

template <typename Arg1, typename... Args>
inline void Logger::debug(std::string_view fmt, const Arg1& arg1, const Args&... args)
{
    log(log_level::debug, fmt, arg1, args...);
}

template <typename Arg1, typename... Args>
inline void Logger::info(std::string_view fmt, const Arg1& arg1, const Args&... args)
{
    log(log_level::info, fmt, arg1, args...);
}

template <typename Arg1, typename... Args>
inline void Logger::warn(std::string_view fmt, const Arg1& arg1, const Args&... args)
{
    log(log_level::warn, fmt, arg1, args...);
}

template <typename Arg1, typename... Args>
inline void Logger::error(std::string_view fmt, const Arg1& arg1, const Args&... args)
{
    log(log_level::err, fmt, arg1, args...);
}

template <typename Arg1, typename... Args>
inline void Logger::fatal(std::string_view fmt, const Arg1& arg1, const Args&... args)
{
    log(log_level::fatal, fmt, arg1, args...);
}

template <typename T>
inline void Logger::trace(const T& msg)
{
    log(log_level::trace, msg);
}

template <typename T>
inline void Logger::debug(const T& msg)
{
    log(log_level::debug, msg);
}

template <typename T>
inline void Logger::info(const T& msg)
{
    log(log_level::info, msg);
}

template <typename T>
inline void Logger::warn(const T& msg)
{
    log(log_level::warn, msg);
}

template <typename T>
inline void Logger::error(const T& msg)
{
    log(log_level::err, msg);
}

template <typename T>
inline void Logger::fatal(const T& msg)
{
    log(log_level::fatal, msg);
}

} // namespace rw::logging

#endif // CSXP_LOGGING_H
