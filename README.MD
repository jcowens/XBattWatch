# XBattWatch

Windows system tray utility for monitoring Xbox controller battery levels

## Features

- Real-time battery level monitoring (Full/Medium/Low/Empty)
- Controller power management
- System tray icon with status indication
- Supports up to 4 connected controllers

## Build Requirements

- MinGW-w64 (GCC 11.2.0+)
- Windows SDK 10.0.20348.0+
- XInput 1.3+ libraries

## Installation

Clone the repository and change to the project directory:

```bash
git clone https://github.com/jcowens/XBattWatch
cd XBattWatch
```

## Building from Source

1. **Install Dependencies**

   On a MinGW system, you can install the toolchain with:

   ```bash
   pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-gcc
   ```

2. **Build Resources and Executable**

   First, compile the resources with `windres`:

   ```bash
   windres resources/resources.rc -O coff -o resources.res
   ```

   Then compile and link the executable using `g++`:

   ```bash
   g++ -o XBattWatch src/main.cpp resources.res -DUNICODE -D_UNICODE -lXinput -mwindows
   ```

## Usage

When launched, XBattWatch runs in the system tray. Right-click the tray icon to:

- Select the active controller
- Turn off controllers
- Exit the application

The tray icon reflects the battery status:

- Full (100%)
- Medium (~50%)
- Low (~20%)
- Empty
