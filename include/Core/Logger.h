#pragma once
#ifndef MIST_LOGGER_H
#define MIST_LOGGER_H

#include <string>
#include <iostream>
#include <sstream>
#include <mutex>
#include <functional>

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERR,
    FATAL
};

class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void SetLevel(LogLevel level) { m_Level = level; }
    LogLevel GetLevel() const { return m_Level; }

    // Secondary sink — called after the default stdout write. The
    // editor installs one here to route WARN/ERROR into the toast
    // notifier. Message is pre-formatted (no trailing newline) and
    // already level-prefixed with file:line on WARN+.
    using Sink = std::function<void(LogLevel, const std::string&)>;
    void SetSink(Sink sink) { m_Sink = std::move(sink); }

    template<typename... Args>
    void Log(LogLevel level, const char* file, int line, Args&&... args) {
        if (level < m_Level) return;
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::ostringstream oss;
        oss << "[" << LevelStr(level) << "] ";
        if (level >= LogLevel::WARN) {
            oss << "(" << file << ":" << line << ") ";
        }
        (oss << ... << std::forward<Args>(args));
        std::string formatted = oss.str();
        std::cout << formatted << "\n";
        if (m_Sink) m_Sink(level, formatted);
    }

private:
    Logger() : m_Level(LogLevel::INFO) {}
    LogLevel m_Level;
    std::mutex m_Mutex;
    Sink m_Sink;

    static const char* LevelStr(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERR:   return "ERROR";
            case LogLevel::FATAL: return "FATAL";
        }
        return "???";
    }
};

#define LOG_TRACE(...) Logger::Instance().Log(LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) Logger::Instance().Log(LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  Logger::Instance().Log(LogLevel::INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  Logger::Instance().Log(LogLevel::WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) Logger::Instance().Log(LogLevel::ERR,   __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) Logger::Instance().Log(LogLevel::FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif // MIST_LOGGER_H
