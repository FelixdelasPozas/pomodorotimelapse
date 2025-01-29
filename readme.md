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
  - A mask can be drawn over the face (choosed over seven different ones).
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
Main dialog with the configuration options showing a preview of the captured area (whole 3840x1080 desktop) and the current position of the overlayed images (both camera and pomodoro statistics). Yeah, I always code with my monocle or my awesome face! ;-P

![interface](https://cloud.githubusercontent.com/assets/12167134/8397664/aa0231c2-1dd3-11e5-94dc-4c00ae932324.jpg)

Once started the tray icon will provide the information on the progression of the timer and the session.

![trayicon](https://cloud.githubusercontent.com/assets/12167134/8397666/aa0c1fde-1dd3-11e5-922b-2107d16183b3.jpg)

If the pomodoro timer is being used the statistics can be shown and the current task can be changed while in a session. 

![statistics](https://cloud.githubusercontent.com/assets/12167134/8397665/aa08269a-1dd3-11e5-864e-ee362e6f52a0.jpg)

# Repository information

**Version**: 1.2.0

**Status**: finished

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |   10          |  754        |   420            | 3474
| C/C++ Header                 |   11          |  328        |    829           | 793  |
| CMake                        |    1          |   20        |      7           |  80  |
| **Total**                    | **22**        | **1102**    | **1256**         | **4347** |
