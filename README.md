![Header-only](https://img.shields.io/badge/header--only-yes-blue.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17%2B-brightgreen.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![License](https://img.shields.io/badge/license-MIT-lightgrey.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-yellow.svg)
![PRs Welcome](https://img.shields.io/badge/PRs-welcome-orange.svg)

# Loggy

**Loggy** is a feature-rich, header-only, RAII-based logging library for modern C++ projects. It provides a thread-safe, customizable logging interface that outputs to the console and a log file. Designed for developer sanity and zero-dependency deployment, Loggy is activated only in debug builds by default.

---

## 🔧 Features

- **Header-only:** Drop-in header, no external dependencies.
- **RAII & Singleton:** Automatic init/deinit, but also supports manual instantiation.
- **Thread-Safe:** Uses `std::mutex` to avoid race conditions.
- **Multiple Log Levels:** `DEBUG`, `INFO`, `WARN`, `ERR`, `FATAL`
- **Customizable Timestamp Format:** Choose your own time representation.
- **Console Output with Colors:** Cross-platform color-coded logs.
- **Optional Console Allocation (Windows):** Dynamically attach a console window in GUI apps.
- **Log File Output:** Log file path is user-defined; supports log rotation at 5MB.
- **Custom Log Handlers:** Hook in your own logging backend.
- **Thread ID in Output:** Know who did the thing.
- **Auto-Flush Mode:** Optional immediate flushing to log file/console.

---

## 💻 Requirements
- C++17 or later

---

## 📦 Integration

### 1. Include the Header
```cpp
#include "Loggy.h"
```

### 2. (Optional) Initialize Console
```cpp
Logger::instance().initializeConsole("My Custom Console Title");
Logger::instance().enableConsoleOutput(true);
```

### 3. Configure (Optional)
```cpp
Logger::instance().setLogPath("logs/app.log");
Logger::instance().setTimestampFormat("%d-%m-%Y %H:%M:%S");
Logger::instance().setLogLevel(LogLevel::DEBUG);
Logger::instance().enableAutoFlush(true);
```

### 4. Log Messages
```cpp
LOG(LogLevel::INFO, "App initialized.");
LOG(LogLevel::DEBUG, "User input: ", inputValue);
```

### 5. Custom Log Handler (Optional)
```cpp
Logger::instance().setCustomLogHandler([](const std::string& msg) {
    // Send to remote service, overlay, etc.
    std::cerr << "[Custom] " << msg << std::endl;
});
```

---

## 🧪 Example
```cpp
#include "Loggy.h"
#include <stdexcept>

int main() {
#ifdef _DEBUG
    Logger::instance().initializeConsole("MyApp [DEBUG]");
    Logger::instance().enableConsoleOutput(true);
    Logger::instance().setLogPath("Main.log");

    LOG(LogLevel::INFO, "Application started");
    int value = 42;
    LOG(LogLevel::DEBUG, "Value: ", value);

    try {
        throw std::runtime_error("Something exploded!");
    } catch (const std::exception& ex) {
        LOG(LogLevel::ERR, "Caught exception: ", ex.what());
    }
#endif
    return 0;
}
```

---

## 🚨 Note on Release Builds

By default, logging is disabled in release builds. Define `LOGGY_DISABLE_LOGGING` to explicitly suppress logging, or remove it to force logging even outside debug mode.

---

## 🧼 Default Behavior Summary
- Console logging is enabled manually.
- File output truncates `Main.log` by default unless changed.
- All logs are timestamped and thread-safe.
- All macros are no-ops in release unless you override.

---

Enjoy your logs. Or at least understand them. That's progress.

