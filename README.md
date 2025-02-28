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

Simply add `Loggy.h` to your project and include it:

```cpp
#include "Loggy.h"
