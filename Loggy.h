#pragma once
#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <iostream>

// Log levels
enum class LogLevel {
    DEBUG_LEVEL,
    INFO,
    WARN,
    _ERROR,
    FATAL
};

/**
 * @brief A header-only, RAII-based logger implemented as a Singleton.
 *
 * This logger, named Loggy, writes log messages to both console and a log file.
 * The log file is created on the user's desktop; if it already exists, its content is overwritten.
 * In release mode, logging is disabled.
 */
class Loggy {
public:
    /// Returns the singleton instance of Loggy.
    static Loggy& instance() {
        // Guaranteed to be destroyed and thread-safe in C++11 and later.
        static Loggy instance;
        return instance;
    }

    // Deleted copy constructor and assignment operator.
    Loggy(const Loggy&) = delete;
    Loggy& operator=(const Loggy&) = delete;

    /**
     * @brief Logs a message with a specific log level.
     *
     * @param level The log level.
     * @param functionName Name of the calling function.
     * @param message The message to log.
     */
    inline void log(LogLevel level, const char* functionName, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (static_cast<int>(level) < static_cast<int>(m_currentLogLevel))
            return;
        std::string levelStr = levelToString(level);
        std::string timeStamp = getCurrentTimeStamp();
        std::ostringstream oss;
        oss << timeStamp << " " << levelStr << " " << functionName << " -> " << message;
        std::string fullMessage = oss.str();

        // Write to console.
        std::cout << fullMessage << std::endl;
        // Write to file if open.
        if (m_logFile.is_open()) {
            m_logFile << fullMessage << std::endl;
        }
    }

    /**
     * @brief Variadic template for logging messages.
     *
     * Concatenates the base message with additional arguments.
     *
     * @param level The log level.
     * @param functionName Name of the calling function.
     * @param message The base message.
     * @param args Additional arguments to be appended.
     */
    template <typename... Args>
    inline void log(LogLevel level, const char* functionName, const std::string& message, Args&&... args) {
        std::ostringstream oss;
        oss << message;
        // Fold expression (requires C++17)
        (oss << ... << args);
        log(level, functionName, oss.str());
    }

    /**
     * @brief Sets the current log level.
     *
     * Only messages with a log level greater or equal to this value will be logged.
     *
     * @param level The new log level.
     */
    inline void setLogLevel(LogLevel level) { m_currentLogLevel = level; }

    /**
     * @brief Sets the console title.
     *
     * Must be called before Loggy is first used.
     *
     * @param title The desired console title.
     */
    static void setConsoleTitle(const std::string& title) {
#ifdef _DEBUG
        s_consoleTitle = title;
        SetConsoleTitleA(s_consoleTitle.c_str());
#endif
    }

private:
    std::ofstream m_logFile;
    std::mutex m_mutex;
    LogLevel m_currentLogLevel = LogLevel::DEBUG_LEVEL;

    // Private static variable for console title.
    inline static std::string s_consoleTitle = "Debug Console";

    // Private constructor because of the singleton pattern.
    Loggy() {
#ifdef _DEBUG
        const char* userProfile = std::getenv("USERPROFILE");
        std::filesystem::path logPath = userProfile ? userProfile : ".";
        logPath /= "Desktop";
        try {
            if (!std::filesystem::exists(logPath))
                std::filesystem::create_directories(logPath);
        }
        catch (const std::filesystem::filesystem_error& err) {
            std::cerr << "Filesystem error: " << err.what() << std::endl;
        }
        
        logPath /= "Main.log";
        m_logFile.open(logPath, std::ios::out | std::ios::trunc);
        if (!m_logFile.is_open())
            std::cerr << "ERROR: could not open log file: " << logPath.string() << std::endl;

        FILE* pFile{};
        if (AllocConsole()) {
            SetConsoleTitleA(s_consoleTitle.c_str());
            freopen_s(&pFile, "CONOUT$", "w", stdout);
            freopen_s(&pFile, "CONOUT$", "w", stderr);
            std::ios::sync_with_stdio(true);
        }
#endif
    }

    // Private destructor.
    ~Loggy() {
#ifdef _DEBUG
        if (m_logFile.is_open()) {
            m_logFile.flush();
            m_logFile.close();
        }
        // Close the standard output streams.
        fclose(stdout);
        fclose(stderr);
        // Free the allocated console.
        FreeConsole();
#endif
    }

    /// Converts a LogLevel to its string representation.
    inline std::string levelToString(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG_LEVEL: return "[ DEBUG ]";
        case LogLevel::INFO:        return "[ INFO  ]";
        case LogLevel::WARN:        return "[ WARN  ]";
        case LogLevel::_ERROR:       return "[ ERROR ]";
        case LogLevel::FATAL:       return "[ FATAL ]";
        default:                    return "[ UNKNOWN ]";
        }
    }

    /// Returns the current timestamp as a formatted string.
    inline std::string getCurrentTimeStamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm timeInfo;
#if defined(_MSC_VER)
        localtime_s(&timeInfo, &now_c);
#else
        localtime_r(&now_c, &timeInfo);
#endif
        std::ostringstream oss;
        oss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};

#ifdef _DEBUG
// Macro to simplify logging usage; automatically uses the current function name.
#define LOG(level, ...) Loggy::instance().log(level, __FUNCTION__, __VA_ARGS__)
#else
#define LOG(level, ...) // No logging in release builds.
#endif
