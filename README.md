# Loggy

**Loggy** is a header-only, RAII-based logging library for C++ projects, implemented as a Singleton.  
It provides an easy-to-use logging interface that writes messages to both the console and a log file on the user's Desktop.  
Loggy is active only in debug builds, ensuring that in release mode minimal traces remain.

## Features

- **Header-only:**  
  Simply include the header in your project—no additional binaries or libraries required.

- **RAII & Singleton:**  
  The logger is automatically initialized on first use and cleaned up on program exit.

- **Thread-Safe Logging:**  
  Uses a mutex to ensure that log messages from multiple threads do not conflict.

- **Multiple Log Levels:**  
  Supports log levels such as `DEBUG_LEVEL`, `INFO`, `WARN`, `_ERROR`, and `FATAL`.

- **Formatted Timestamps:**  
  Each log message is prefixed with a timestamp in the format `YYYY-MM-DD HH:MM:SS`.

- **Log File on Desktop:**  
  The log file is created on the user's Desktop as `Main.log`. If the file already exists, its content is overwritten.

## Requirements

C++17+

## Usage

### Integration

1. **Include the Header**

Simply add `Loggy.h` to your project and include it:

```cpp
#include "Loggy.h"
```

2. **Set the Console Title (Optional)**

If you want to customize the console title, call `Loggy::setConsoleTitle`:

```cpp
Loggy::setConsoleTitle("My Custom Console Title [DEBUG]");
```

3. **Log Messages**

Use the `LOG` macro to log messages. This macro automatically passes the calling function's name:

```cpp
LOG(LogLevel::INFO, "Application started");
```

For variadic logging, simply pass additional parameters:

```cpp
int value = 42;
LOG(LogLevel::DEBUG_LEVEL, "The value is: ", std::dec, value);
```

4. **Build Configuration**

Ensure your project is compiled with `_DEBUG` defined to enable logging (already defined by Visual Studio). In release builds, the logging macros are disabled so that no log output is generated.

## Example

Below is a small example illustrating the usage:

```cpp
#include "Loggy.h"
#include <stdexcept>

int main() {
#ifdef _DEBUG
    // Optionally set a custom console title before any logging occurs.
    Loggy::setConsoleTitle("MyApp [DEBUG]");

    // Log some messages.
    LOG(LogLevel::INFO, "Application started");

    int value = 42;
    LOG(LogLevel::DEBUG_LEVEL, "Value: ", value);

    try {
        throw std::runtime_error("An error occurred!");
    } catch (const std::exception& ex) {
        LOG(LogLevel::_ERROR, "Exception caught: ", ex.what());
    }
#endif

    return 0;
}
```
