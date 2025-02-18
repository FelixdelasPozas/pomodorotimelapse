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

DesktopCapture is available for Windows 10 onwards. You can download the latest installer from the [releases page](https://github.com/FelixdelasPozas/pomodorotimelapse/releases). Neither the application or the installers are digitally signed so the system will ask for approval before running it the first time.

The last version supporting Windows 7 & 8 is 1.2.0, you can download it [here](https://github.com/FelixdelasPozas/pomodorotimelapse/releases/tag/1.2.0).

# Screenshots

> [!NOTE] 
> Some screenshots are from the Windows 7 version. 

Main dialog with the configuration options showing a preview of the captured area (whole 3840x1080 desktop) and the current position of the overlayed images (both camera and pomodoro statistics). Yeah, I always code with my monocle or my awesome face! ;-P

![interface](https://cloud.githubusercontent.com/assets/12167134/8397664/aa0231c2-1dd3-11e5-94dc-4c00ae932324.jpg)
![interface2](https://github.com/user-attachments/assets/ac6014ed-e977-4f51-800d-2456d40fb01c)

Once started the tray icon will provide the information on the progression of the timer and the session.

![trayicon](https://cloud.githubusercontent.com/assets/12167134/8397666/aa0c1fde-1dd3-11e5-922b-2107d16183b3.jpg)

If the pomodoro timer is being used the statistics can be shown and the current task can be changed while in a session. 

![statistics](https://cloud.githubusercontent.com/assets/12167134/8397665/aa08269a-1dd3-11e5-864e-ee362e6f52a0.jpg)

All the options present in the main dialog to control the presence and position of the desktop widgets. 

![options](https://github.com/user-attachments/assets/c26a8fa9-6c5d-4025-a1e9-75a7d3b12487)

The three options of the ASCII art camera image.

![asciiart](https://github.com/user-attachments/assets/8246580c-2c05-4cee-b91c-e980b143703c)

# Repository information

**Version**: 1.3.0

**Status**: finished

**cloc statistics**

| Language                     |files          |blank        |comment      |code  |
|:-----------------------------|--------------:|------------:|------------:|-----:|
| C++                          |   10          |  849        |   452       | 3601 |
| C/C++ Header                 |   11          |  375        |   966       | 939  |
| CMake                        |    1          |   19        |     4       |  75  |
| **Total**                    | **22**        | **1243**    | **1422**    | **4615** |

