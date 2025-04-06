#pragma once
#define NOMINMAX
#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <string>
#include <filesystem>
#include <iostream>
#include <functional>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#endif

constexpr std::size_t MAX_LOG_FILE_SIZE = 5 * 1024 * 1024; // 5 MB

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERR,
    FATAL
};

class Logger {
public:
    Logger() = default;

    void setLogPath(const std::filesystem::path& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logFilePath = path;
        openLogFile();
    }

    void enableConsoleOutput(bool enable) {
        m_consoleOutput = enable;
    }

    void enableAutoFlush(bool enable) {
        m_autoFlush = enable;
    }

    void initializeConsole(const std::string& title = "Loggy Console") {
#ifdef _WIN32
        if (AllocConsole()) {
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            freopen_s(&dummy, "CONOUT$", "w", stderr);
            SetConsoleTitleA(title.c_str());
            std::ios::sync_with_stdio();
        }
#endif
    }

    void setLogLevel(LogLevel level) {
        m_currentLogLevel = level;
    }

    void setTimestampFormat(const std::string& format) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_timeFormat = format;
    }

    void setCustomLogHandler(std::function<void(const std::string&)> handler) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_customHandler = std::move(handler);
    }

    void log(LogLevel level, const char* functionName, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (level < m_currentLogLevel) return;

        std::string output = formatLogMessage(level, functionName, message);

        if (m_customHandler) {
            m_customHandler(output);
        }
        else {
            if (m_consoleOutput) {
                setConsoleColor(level);
                std::cout << output << std::endl;
                if (m_autoFlush) std::cout << std::flush;
                resetConsoleColor();
            }

            if (m_logFile.is_open()) {
                rotateLogIfNeeded();
                m_logFile << output << std::endl;
                if (m_autoFlush) m_logFile.flush();
            }
        }
    }

    template <typename... Args>
    void log(LogLevel level, const char* functionName, const std::string& message, Args&&... args) {
        std::ostringstream oss;
        oss << message;
        (oss << ... << args);
        log(level, functionName, oss.str());
    }

    static Logger& instance() {
        static Logger instance;
        return instance;
    }

private:
    std::ofstream m_logFile;
    std::filesystem::path m_logFilePath;
    std::mutex m_mutex;
    LogLevel m_currentLogLevel = LogLevel::DEBUG;
    bool m_consoleOutput = true;
    bool m_autoFlush = false;
    std::string m_timeFormat = "%Y-%m-%d %H:%M:%S";
    std::function<void(const std::string&)> m_customHandler = nullptr;

    void openLogFile() {
        m_logFile.open(m_logFilePath, std::ios::out | std::ios::trunc);
        if (!m_logFile)
            std::cerr << "[Logger] Failed to open log file at: " << m_logFilePath << std::endl;
    }

    void rotateLogIfNeeded() {
        if (std::filesystem::exists(m_logFilePath)) {
            auto fileSize = std::filesystem::file_size(m_logFilePath);
            if (fileSize > MAX_LOG_FILE_SIZE) {
                m_logFile.close();
                std::filesystem::remove(m_logFilePath);
                openLogFile();
            }
        }
    }

    std::string formatLogMessage(LogLevel level, const char* function, const std::string& msg) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#if defined(_MSC_VER)
        localtime_s(&tm, &now_c);
#else
        localtime_r(&now_c, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, m_timeFormat.c_str())
            << " [Thread: " << std::this_thread::get_id() << "]"
            << " [" << logLevelToString(level) << "] "
            << function << " -> " << msg;
        return oss.str();
    }

    std::string logLevelToString(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
        }
    }

    void setConsoleColor(LogLevel level) {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD color;
        switch (level) {
        case LogLevel::DEBUG: color = FOREGROUND_BLUE | FOREGROUND_GREEN; break;  // cyan
        case LogLevel::INFO: color = FOREGROUND_GREEN; break;
        case LogLevel::WARN: color = FOREGROUND_RED | FOREGROUND_GREEN; break; // yellow
        case LogLevel::ERR: color = FOREGROUND_RED; break;
        case LogLevel::FATAL: color = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        default: color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        }
        SetConsoleTextAttribute(hConsole, color);
#else
        switch (level) {
        case LogLevel::DEBUG: std::cout << "\033[36m"; break; // cyan
        case LogLevel::INFO: std::cout << "\033[32m"; break; // green
        case LogLevel::WARN: std::cout << "\033[33m"; break; // yellow
        case LogLevel::ERR: std::cout << "\033[31m"; break; // red
        case LogLevel::FATAL: std::cout << "\033[1;31m"; break; // bright red
        default: break;
        }
#endif
    }

    void resetConsoleColor() {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
        std::cout << "\033[0m";
#endif
    }
};

#ifndef LOGGY_DISABLE_LOGGING
#define LOG(level, ...) Logger::instance().log(level, __FUNCTION__, __VA_ARGS__)
#else
#define LOG(level, ...) ((void)0)
#endif
