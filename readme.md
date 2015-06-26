Desktop Capture & Pomodoro Timer
================================

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
DesktopCapture is a tool to create a video or a serie of images of the desktop activity, taken at given intervals. It's similar to many chronolapse/time-lapse tools available on the internet but with a few quirks.

## Options
Several options can be configured:
* the region of the desktop to be captured can be defined as the whole desktop or a single monitor.
* if a webcam is detected the image can be overlayed over the desktop captured image.
* implements the pomodoro technique and the statistics from the session can be overlayed over the desktop captured image (it can be used just as a pomodoro timer if the desktop capture is turned off). 
* sounds for the pomodoro events with the option to mute it completely or enable the continuous tic tac of the clock during the whole session. 
* the output video or images can be scaled in size.
* the overlayed camera and pomodoro images con be configured in position (freely or one of the nine fixed positions) and composition mode. 

## Just for fun options
Also implemented some features just because they we're easy to do and fun.
* The captured camera image can be converter to ASCII art.
* Also if a face is detected in the camera picture:
  - A mask can be drawn over (choosed over seven different ones).
  - The face can be centered in the camera image. 

## Output file formats
The tool outputs the following files:
* **Video format**: Google's webm video.
* **Image format**: a series of PNG files. 

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

## External dependencies
The following libraries are required:
* [libvpx](https://chromium.googlesource.com/webm/libvpx) - WebM VP8/VP9 Codec SDK
* [libyuv](https://code.google.com/p/libyuv/) - YUV conversion and scaling functionality
* [libdlib](http://dlib.net) - dLib C++ library.
* [OpenCV](http://opencv.org) - Open source computer vision library.
* [Qt opensource framework](http://www.qt.io/).

# Install
The only current option is build from source as binaries are not provided. 

# Screenshots

Coming soon...

# Repository information

**Version**: 1.0.0

**Status**: finished

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |    9          |  733        |    400           |3432  |
| C/C++ Header                 |   10          |  314        |    767           | 761  |
| CMake                        |    1          |   18        |      5           |  79  |
| **Total**                    | **20**        | **1065**    | **1172**         | **4272** |


