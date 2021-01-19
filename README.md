# libdbase

**NOTE:** This library was originally created about 10 years ago. I no longer have access to the relevant hardware and it appears that ORTEC has silently updated the original hardware (see this repo: [https://github.com/kjbilton/libdbaserh](https://github.com/kjbilton/libdbaserh) and thus this library might not be compatible with your device.

- libdbase is a C library for (USB) ORTEC digiBASE access from Linux userspace. 
  libdbase is licensed under the GPL-3 (see LICENSE).

- libdbase uses libusb-1.0 for lowlevel USB communication.
  This library should hence work on all platforms that libusb-1.0 is ported to.
  See libusb homepage for more information on libusb:
      http://libusb.sourceforge.net

- For more information on libdbase, see the project homepage on sourceforge:
      http://libdbase.sourceforge.net

## Binary dependecy
**NOTE:** We have not the right to distribute the non-free firmware you need to use this library. But since you probably already have purchased a copy of Maestro (or similar program included with digibase) you are free to copy the firmware  from one installation (Windows) to another (Linux), as long as you don't redistribute it.

  1. Locate _digiBase.rbf_ on your Windows system with the Ortec program installed.
  (default installation path seems to be c:\windows\system32\digiBase.rbf)
  2. `md5sum` of _digiBase.rbf_ should be: 
  f1846928a2c233c318e725658e638d6e
  3. Copy this file to your Linux system.

## Installation
  1. Install libusb-1.0
  2. Move the _digiBase.rbf_ file to the root of the libdbase project. Alternatively, in the next step, you can set the path to the file with the argument (e.g.) `-DDIGIBASE_RBF_PATH=/path/to/file/`.
  3. Create a build directory and run CMake: `mkdir build && cmake ..`.
  4. Compile the library and examples: `make`.
  5. Install the library `sudo make install`.
  6. Run on of the applications, e.g.: 'sudo ./example1'

## Manual
  I haven't had the time to put together a comprehensive manual of available functions,
  so I can only refer to the public header file: src/libdbase.h. In src/example*.c and 
  src/dbase.c the libdbase is put into use.

## To-do List:
**Note:** This has been copied from the original documentation and is unlikely to be completed.

- Display actual HV
- Handle status messages (are there any?)
- Support for digiBASE-RH (dual EPs)
- Support for digiDART
- Support for Detective...


## Changes

### Version 0.3

- Use CMake to generate a build system.
- Added a simple C++ interface, originally developed to simplify the use of this library with Python.
- Minor fix to event-mode code.
- Re-formatted the code with clang-format.


### Version 0.2

- cleaned up namespace
- save/load status (text) functionality
- detector** list functionality added to enable initialization of several dbase's at once
- live/real time preset functionality
- some minor bugs solved
