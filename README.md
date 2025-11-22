![Header-only](https://img.shields.io/badge/header--only-yes-blue.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17%2B-brightgreen.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![License](https://img.shields.io/badge/license-MIT-lightgrey.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-yellow.svg)
![PRs Welcome](https://img.shields.io/badge/PRs-welcome-orange.svg)

# Loggy

**Loggy** is a feature-rich, header-only logging library for modern C++ projects. It is thread-safe, easily configurable, and writes to both console and file. Zero-dependency (standard library). Colored output is currently only available on Windows (via WinAPI). Rotations and filters can be set via macros.

---

## 🔧 Features (current state)
- Header-only (`loggy.hpp`).
- Thread-safe (Mutex, optional best-effort trylock to skip on contention).
- Singleton access: `Logger::instance()` (manual shutdown possible).
- Multiple log levels: `DEBUG, INFO, WARN, ERR, FATAL` (output of ERR as `ERROR`).
- Compile-time level filter via `LOGGY_MIN_LEVEL` (0=DEBUG .. 4=FATAL).
- Runtime level filter via `setLogLevel(LogLevel)`.
- Optional file rotation: Size via `LOGGY_MAX_LOG_FILE_SIZE` (Default 5MB) + backups `LOGGY_ROTATE_BACKUPS`.
- Check interval for rotation via `LOGGY_CHECK_INTERVAL` (lines).
- Switchable outputs: `enableConsoleOutput()`, `enableFileOutput()`, `enableAutoFlush()`.
- Custom timestamp format: `setTimestampFormat()` (Default `%Y-%m-%d %H:%M:%S`).
- Thread ID can be shown/hidden: `includeThreadId(bool)` (Default on).
- Function & (optional) file:line in output (`LOG_EX` or C++20 `source_location` variant).
- Custom handler: `setCustomLogHandler(fn)`.
- Windows: automatic console allocation via `initializeConsole()` (for GUI apps).
- Color output (Windows only) controllable via `LOGGY_COLORIZE_CONSOLE`.
- Scope Timer Utility: `LogScopeTimer` measures duration and logs on destruction.
- Best performance option: `LOGGY_BEST_EFFORT_TRYLOCK` (set to 1 to discard log on mutex block).

---

## Requirements
- C++ 17 (optionally uses `std::format` if available >= C++ 23 implementation).

---

## Integration

### 1. Include
```cpp
#include "loggy.hpp"
```

### 2. (Optional, Windows) Create Console
```cpp
Logger::instance().initializeConsole("My Window");
```

### 3. Basic Configuration
```cpp
auto& L = Logger::instance();
L.setLogPath("logs/app.log");           // creates/opens file (truncates on first set)
L.setTimestampFormat("%d-%m-%Y %H:%M:%S");
L.setLogLevel(LogLevel::DEBUG);          // Runtime min-level
L.includeThreadId(true);                 // Default is already true
L.enableAutoFlush(true);                 // flush immediately
// Optionally disable outputs
// L.enableConsoleOutput(false);
// L.enableFileOutput(false);
```

### 4. Logging Macros
```cpp
LOG(LogLevel::INFO, "Start");
LOG(LogLevel::DEBUG, "Value = ", value);
LOG_EX(LogLevel::WARN, "Something happened: ", code); // includes file & line
```

The `LOG` macro automatically includes the function name. `LOG_EX` additionally includes file & line.

### 5. C++20 Variant (source_location)
When compiling with C++20 or newer, an overloaded `log` method can be used, which automatically captures file, line, and function:
```cpp
// No __FUNCTION__ or __FILE__/__LINE__ needed
Logger::instance().log(LogLevel::INFO, "Hello from new variant");
```

### 6. Custom Handler
```cpp
Logger::instance().setCustomLogHandler([](const std::string& line){
    // e.g., send remotely
    std::cerr << "[CUSTOM] " << line << '\n';
});
```

### 7. Scope Timer
```cpp
{
    LogScopeTimer t("expensiveOperation");
    // ... work
} // Automatically logs duration in microseconds
```

### 8. Shutdown (optional)
```cpp
Logger::instance().shutdown(); // flush & close
```

---

## Configuration Macros
Definable at compile-time (e.g., via compiler flags):
- `LOGGY_DISABLE_LOGGING` disables all LOG / LOG_EX macros.
- `LOGGY_MIN_LEVEL` Compile-time minimum level (Default 0 = DEBUG).
- `LOGGY_MAX_LOG_FILE_SIZE` Bytes until rotation (Default 5*1024*1024).
- `LOGGY_ROTATE_BACKUPS` Number of backups (Default 3 -> file, file.1, .2, .3).
- `LOGGY_CHECK_INTERVAL` Line interval for size check (Default 200).
- `LOGGY_COLORIZE_CONSOLE` 1/0 for color output (Windows only).
- `LOGGY_BEST_EFFORT_TRYLOCK` 1 to skip log on mutex contention.

Runtime adjustments (public methods of the `Logger` class) provide additional control over behavior.

---

## Example
```cpp
#include "loggy.hpp"
#include <stdexcept>

int main() {
    Logger::instance().initializeConsole("App Console"); // optional (Windows)
    Logger::instance().setLogPath("Main.log");

    LOG(LogLevel::INFO, "Application started");

    int value = 42;
    LOG(LogLevel::DEBUG, "Value: ", value);

    try {
        throw std::runtime_error("Explosion!");
    } catch (const std::exception& ex) {
        LOG_EX(LogLevel::ERR, "Caught exception: ", ex.what());
    }

    {
        LogScopeTimer t("WorkBlock");
        // simulate work
    }

    Logger::instance().shutdown();
    return 0;
}
```

---

## Build Notes
Logging is always active unless `LOGGY_DISABLE_LOGGING` is defined. For lower cost in Release builds:
- Set `LOGGY_MIN_LEVEL` higher (e.g., 1 for INFO).
- Increase the runtime level via `setLogLevel()`.
- For very high concurrency, set `LOGGY_BEST_EFFORT_TRYLOCK` to 1 to avoid blockages.

---

## Default Behavior
- Console & File output are active by default (File only effective after `setLogPath`).
- Timestamp format: `%Y-%m-%d %H:%M:%S`.
- Thread ID is included (can be disabled).
- Function name is always in the output (macros). File & line only with `LOG_EX` or the C++20 variant.
- Rotation: by size > 5MB (3 backups) at check intervals (200 lines).
- ERR level text appears as `ERROR`.
- Color output on Windows only, if enabled.

---
