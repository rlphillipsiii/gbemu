## Overview
![](https://github.com/rlphillipsiii/gbemu/workflows/Linux%20CI/badge.svg)
![](https://github.com/rlphillipsiii/gbemu/workflows/MacOS%20CI/badge.svg)

##### Supported Platforms
* Linux Based OS's (i.e. Ubuntu)
* Windows 10

Might also work on a Mac but has never been tested.

## Requirements
* QT5.8 or newer
* g++ (MinGW64 on Windows)
* GIT Bash (Windows only)
* Cygwin/MobaXterm (Windows only)

## Building from Source
Open up a terminal emulator (I recommend using the git bash shell on Windows) and run the following command:
```sh
qmake -r CONFIG+=debug_and_release
```

Build using the following command (I recommend using Cygwin or MobaXterm on Windows):
```sh
make <debug|release>
```

Use the "debug" to build a debug build.  Debug builds are built with debugging symbols enabled and compiler optimization level 0 (no optimizations).  Debug builds also "DEBUG" defined, whereas release builds have "RELEASE" defined.  This allows for conditional code compilation based on the build configuration:
```c++
#ifdef DEBUG
    printf("This is only compiled for a debug build\n");
#endif
#ifdef RELEASE
    printf("This is only compiled for a release build\n");
#endif
```

## Code Architecture and Organization
The backend (hardware emulation) and frontend (display and IO) are separated in to two separate modules.  Any code in the "hardware" directory belongs to the backend, and any code in the "ui" directory belongs to the frontend.

This is a cross platfrom emulator (which is why the UI is using QT).  This implies that platform specific code is not allowed unless there is an equivalent implementation in all supported platforms (or at least a way to exit gracefully if a feature is not implemented on a particular platform.

##### Backend Code
The backend is responsible for the core hardware emulation functionality.  This section of the code should have no dependencies on whatever rendering mechanism the UI code uses to paint pixels on the screen.  At this point in time, this specifically means that no QT dependent code in the backend.  Any infrastructure, data types, data structures, etc should not rely or have any knowledge of the UI framework being used by the front end and must be able to build regardless of whether or not the frontend code is even present.  This implies that no headers published from the UI directory should be included in a source file located in the hardware directory.

Each major piece of hardware that needs to be emulated by the backend has its own class:
* Processor: Responsible for the emulation of the gbc's Z8080 like CPU.
* GPU: Responsible for implementing the gbc's GPU functionality.  It also contains the logic to translate the pixels as they are represented in the GPU ram to a RGBA8888 pixel array to be consumed by the UI rendering code.
* MemoryController: Responsible for emulation the gbc's memory operations, which includes reads, writes, and memory mapped IO.

##### Frontend Code
Written in QT and only communicates with the backend code via the backend layer's public interface.  The frontend is only responsible for drawing a scaled version of the RGBA8888 pixel array that is provided by the backend and passing IO to the backend.

## References Documents
* http://bgb.bircd.org/pandocs.htm
