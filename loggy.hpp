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
#include <atomic>
#include <source_location>
#if __cpp_lib_format >= 202207L
    #include <format>
#endif

#ifdef _WIN32
    #include <windows.h>
    #include <cstdio>
#endif

#ifndef LOGGY_MAX_LOG_FILE_SIZE
#   define LOGGY_MAX_LOG_FILE_SIZE (5ull * 1024ull * 1024ull) // 5 MB
#endif

#ifndef LOGGY_ROTATE_BACKUPS
#   define LOGGY_ROTATE_BACKUPS 3                             // keep N old files: file, file.1, file.2, ...
#endif

#ifndef LOGGY_CHECK_INTERVAL
#   define LOGGY_CHECK_INTERVAL 200                           // check file size every N messages
#endif

#ifndef LOGGY_MIN_LEVEL
#  define LOGGY_MIN_LEVEL 0                                   // 0=DEBUG, 1=INFO, 2=WARN, 3=ERR, 4=FATAL
#endif

#ifndef LOGGY_COLORIZE_CONSOLE
#  define LOGGY_COLORIZE_CONSOLE 1
#endif

#ifndef LOGGY_BEST_EFFORT_TRYLOCK
#  define LOGGY_BEST_EFFORT_TRYLOCK 0                         // set to 1 to skip logs when mutex contended
#endif

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERR,
    FATAL
};

// Compile-time filter
constexpr bool loggy_enabled(LogLevel lvl) {
    return static_cast<int>(lvl) >= LOGGY_MIN_LEVEL;
}

// -----------------------------
// Logger
// -----------------------------
class Logger {
public:
    Logger() = default;

    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    // Basic setup
    void setLogPath(const std::filesystem::path& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logFilePath = path;
        ensureDir(path);
        openLogFile(/*truncate=*/true);
    }

    void enableConsoleOutput(bool enable) noexcept { m_consoleOutput.store(enable, std::memory_order_relaxed); }
    void enableFileOutput(bool enable)    noexcept { m_fileOutput.store(enable, std::memory_order_relaxed); }
    void enableAutoFlush(bool enable)     noexcept { m_autoFlush.store(enable, std::memory_order_relaxed); }
    void setLogLevel(LogLevel level)      noexcept { m_runtimeMinLvl.store(level, std::memory_order_relaxed); }
    void includeThreadId(bool on)         noexcept { m_includeThreadId.store(on, std::memory_order_relaxed); }

    void setTimestampFormat(const std::string& format) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_timeFormat = format;
    }

    void setCustomLogHandler(std::function<void(const std::string&)> handler) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_customHandler = std::move(handler);
    }

    void shutdown() noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_logFile.is_open()) {
            m_logFile.flush();
            m_logFile.close();
        }
    }

    void initializeConsole(const std::string& title = "Loggy Console") {
#ifdef _WIN32
        // if there's no console: acquire one
        if (GetConsoleWindow() == nullptr) {
            // try: attach to parent process (in case it has a console)
            if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
                AllocConsole();
            }
        }

        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);

        if (!title.empty()) SetConsoleTitleA(title.c_str());

        std::ios::sync_with_stdio(true);
        std::cin.tie(nullptr);

        // delete old error-states
        std::cout.clear(); std::cerr.clear(); std::wcout.clear(); std::wcerr.clear();
#else
        (void)title;
#endif
    }

    // Core log function (stream-style variadic)
    template <typename... Args>
    void log(LogLevel level, const char* functionName, const std::string& message, Args&&... args) {
        if (!loggy_enabled(level)) return;
        if (level < m_runtimeMinLvl.load(std::memory_order_relaxed)) return;

        // Use ostringstream for variadic arguments (works reliably for all types)
        std::ostringstream oss;
        oss << message;
        (oss << ... << args);
        submit(level, functionName, nullptr, 0, oss.str());
    }

    // Extended: include file:line
    template <typename... Args>
    void logEx(LogLevel level, const char* functionName, const char* file, int line,
        const std::string& message, Args&&... args)
    {
        if (!loggy_enabled(level)) return;
        if (level < m_runtimeMinLvl.load(std::memory_order_relaxed)) return;

        // Use ostringstream for variadic arguments (works reliably for all types)
        std::ostringstream oss;
        oss << message;
        (oss << ... << args);
        submit(level, functionName, file, line, oss.str());
    }

    // Simple convenience
    void log(LogLevel level, const char* functionName, const std::string& message) {
        if (!loggy_enabled(level)) return;
        if (level < m_runtimeMinLvl.load(std::memory_order_relaxed)) return;
        submit(level, functionName, nullptr, 0, message);
    }

    // Modern C++20/23 variant using source_location
    template <typename... Args>
    void log(LogLevel level, const std::string& message, Args&&... args,
        const std::source_location& loc = std::source_location::current()) {
        if (!loggy_enabled(level)) return;
        if (level < m_runtimeMinLvl.load(std::memory_order_relaxed)) return;

        // Use ostringstream for variadic arguments (works reliably for all types)
        std::ostringstream oss;
        oss << message;
        (oss << ... << args);
        submit(level, loc.function_name(), loc.file_name(), static_cast<int>(loc.line()), oss.str());
    }

private:
    inline static thread_local bool m_inHandler = false;

    std::ofstream m_logFile;
    std::filesystem::path m_logFilePath;
    std::mutex m_mutex;

    std::atomic<size_t> m_lineCount{ 0 };

    std::atomic<LogLevel> m_runtimeMinLvl{ LogLevel::DEBUG };
    std::atomic<bool> m_consoleOutput{ true };
    std::atomic<bool> m_fileOutput{ true };
    std::atomic<bool> m_autoFlush{ false };
    std::atomic<bool> m_includeThreadId{ true };

    std::string m_timeFormat = "%Y-%m-%d %H:%M:%S";
    std::function<void(const std::string&)> m_customHandler = nullptr;

    // ---- core submit path with locking ----
    void submit(LogLevel level, const char* func, const char* file, int line, const std::string& msg) {
        std::string out;
        std::function<void(const std::string&)> handler;
        bool doConsole = false;
        bool doFile = false;
        bool autoFlush = false;
        bool includeThreadId = false;
        std::string timeFormat;

#if LOGGY_BEST_EFFORT_TRYLOCK
        if (!m_mutex.try_lock()) return;
        std::unique_lock<std::mutex> lock(m_mutex, std::adopt_lock);
#else
        std::unique_lock<std::mutex> lock(m_mutex);
#endif
        {
            // Format and copy necessary state while holding the lock
            includeThreadId = m_includeThreadId.load(std::memory_order_relaxed);
            timeFormat = m_timeFormat;
            out = formatLine(level, func, file, line, msg, includeThreadId, timeFormat);
            handler = m_customHandler;
            doConsole = m_consoleOutput.load(std::memory_order_relaxed);
            doFile = m_fileOutput.load(std::memory_order_relaxed);
            autoFlush = m_autoFlush.load(std::memory_order_relaxed);
        }
        lock.unlock(); // Release lock early for I/O operations

        if (handler && !m_inHandler) {
            m_inHandler = true;
            try { handler(out); }
            catch (...) {}
            m_inHandler = false;
        }

#if defined(_WIN32) && LOGGY_COLORIZE_CONSOLE
        if (doConsole) {
            setConsoleColor(level);
            std::cout << out << '\n';
            if (autoFlush) std::cout.flush();
            resetConsoleColor();
        }
#else
        if (doConsole) {
            std::cout << out << '\n';
            if (autoFlush) std::cout.flush();
        }
#endif

        if (doFile) {
            lock.lock(); // Re-acquire lock for file operations
            if (m_logFile.is_open()) {
                if (++m_lineCount % LOGGY_CHECK_INTERVAL == 0) rotateIfNeeded();
                m_logFile << out << '\n';
                if (autoFlush) m_logFile.flush();
            }
        }
    }

    // ---- formatting ----
    std::string formatLine(LogLevel level, const char* func, const char* file, int line,
        const std::string& msg, bool includeThreadId, const std::string& timeFormat) const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_MSC_VER)
        if (localtime_s(&tm, &now_c) != 0) {
            // Error handling: use current time as fallback
            std::time(&now_c);
            localtime_s(&tm, &now_c);
        }
#else
        if (localtime_r(&now_c, &tm) == nullptr) {
            // Error handling: use current time as fallback
            std::time(&now_c);
            localtime_r(&now_c, &tm);
        }
#endif

#if __cpp_lib_format >= 202207L
        try {
            std::ostringstream time_oss;
            time_oss << std::put_time(&tm, timeFormat.c_str());
            std::string formatted = time_oss.str();
            formatted += std::format(" [{}]", levelToStr(level));
            if (includeThreadId) {
                formatted += std::format(" [T:{}]", std::this_thread::get_id());
            }
            if (file && *file) {
                formatted += std::format(" [{}:{}]", file, line);
            }
            if (func && *func) {
                formatted += std::format(" {} -> {}", func, msg);
            }
            else {
                formatted += std::format(" {}", msg);
            }
            return formatted;
        }
        catch (...) {
            // Fallback to ostringstream
        }
#endif
        // Fallback to ostringstream (always available or if format failed)
        std::ostringstream oss;
        oss << std::put_time(&tm, timeFormat.c_str())
            << " [" << levelToStr(level) << "]";
        if (includeThreadId) {
            oss << " [T:" << std::this_thread::get_id() << "]";
        }
        if (file && *file) {
            oss << " [" << file << ":" << line << "]";
        }
        if (func && *func) {
            oss << " " << func << " -> ";
        }
        else {
            oss << " ";
        }
        oss << msg;
        return oss.str();
    }

    [[nodiscard]] constexpr const char* levelToStr(LogLevel level) const noexcept {
        switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERR:   return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKNOWN";
        }
    }

    // ---- console color helpers ----
    void setConsoleColor(LogLevel level) {
#if defined(_WIN32) && LOGGY_COLORIZE_CONSOLE
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        switch (level) {
        case LogLevel::DEBUG: color = FOREGROUND_BLUE | FOREGROUND_GREEN; break;           // cyan
        case LogLevel::INFO:  color = FOREGROUND_GREEN; break;
        case LogLevel::WARN:  color = FOREGROUND_RED | FOREGROUND_GREEN; break;            // yellow
        case LogLevel::ERR:   color = FOREGROUND_RED; break;
        case LogLevel::FATAL: color = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        default: break;
        }
        SetConsoleTextAttribute(h, color);
#else
        (void)level;
#endif
    }

    void resetConsoleColor() {
#if defined(_WIN32) && LOGGY_COLORIZE_CONSOLE
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
    }

    // ---- file helpers ----
    static void ensureDir(const std::filesystem::path& fp) noexcept {
        std::error_code ec;
        const auto dir = fp.has_parent_path() ? fp.parent_path() : std::filesystem::current_path();
        std::filesystem::create_directories(dir, ec);
        // Ignore errors silently (best effort)
    }

    void openLogFile(bool truncate) {
        std::ios::openmode mode = std::ios::out;
        if (!truncate) mode |= std::ios::app;
        m_logFile.open(m_logFilePath, mode);
        if (!m_logFile) {
            std::cerr << "[Loggy] Failed to open log file: " << m_logFilePath << std::endl;
        }
        m_lineCount = 0;
    }

    void rotateIfNeeded() noexcept {
        if (!m_fileOutput.load(std::memory_order_relaxed)) return;
        std::error_code ec;
        if (!std::filesystem::exists(m_logFilePath, ec)) return;

        auto sz = std::filesystem::file_size(m_logFilePath, ec);
        if (ec || sz <= LOGGY_MAX_LOG_FILE_SIZE) return;

        m_logFile.flush();
        m_logFile.close();

#if LOGGY_ROTATE_BACKUPS > 0
        for (int i = LOGGY_ROTATE_BACKUPS - 1; i >= 1; --i) {
            auto from = m_logFilePath;
            from += "." + std::to_string(i);
            auto to = m_logFilePath;
            to += "." + std::to_string(i + 1);
            if (std::filesystem::exists(from, ec)) {
                std::filesystem::remove(to, ec);
                std::filesystem::rename(from, to, ec);
            }
        }
        auto first = m_logFilePath;
        first += ".1";
        std::filesystem::remove(first, ec);
        std::filesystem::rename(m_logFilePath, first, ec);
#endif
        openLogFile(/*truncate=*/true);
    }
};

// -----------------------------
// Macros
// -----------------------------
#ifndef LOGGY_DISABLE_LOGGING
    #define LOG(level, ...)   do { Logger::instance().log((level), __FUNCTION__, __VA_ARGS__); } while(0)
    #define LOG_EX(level, ...) do { Logger::instance().logEx((level), __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__); } while(0)
#else
    #define LOG(level, ...)    ((void)0)
    #define LOG_EX(level, ...) ((void)0)
#endif

// -----------------------------
// Scoped timer helper
// -----------------------------
class LogScopeTimer {
public:
    explicit LogScopeTimer(const char* what, LogLevel level = LogLevel::DEBUG) noexcept
        : m_what(what), m_level(level), m_start(std::chrono::steady_clock::now()) {
    }

    ~LogScopeTimer() noexcept(false) {
        using namespace std::chrono;
        auto dur = duration_cast<microseconds>(steady_clock::now() - m_start).count();
        Logger::instance().log(m_level, m_what, std::string("took "), dur, "us");
    }

    // Prevent copying
    LogScopeTimer(const LogScopeTimer&) = delete;
    LogScopeTimer& operator=(const LogScopeTimer&) = delete;

private:
    const char* m_what;
    LogLevel m_level;
    std::chrono::steady_clock::time_point m_start;
};
