Desktop Capture & Pomodoro Timer
================================
DesktopCapture is a tool to create a video or a serie of images of the desktop activity, taken at given intervals. It's similar to many ''chronolapse'' tools available on the internet but with a few quirks.

Optionally:
* the region of the desktop to be captured can be defined as the whole desktop or a single monitor.
* if a webcam is detected the image can be overlayed over the desktop captured image.
* implements the pomodoro technique and the statistics from the session can be overlayed over the desktop captured image (it can be used just as a pomodoro timer if the desktop capture is turned off). 
* the output can be scaled in size.
* the overlayed images con be configured in position (freely or one of the nine fixed positions) and composition mode. 

# Just for fun options

Also implemented some features just because they we're easy and fun.
* The captured camera image can be converter to ASCII art.
* Also if a face is detected in the camera picture:
  - A mask can be drawn over.
  - The face can be centered in the camera image. 

# Output file formats
The tool outputs the following files:
* **Video format** : Google's webm video.
* **Image format** : a series of PNG files. 

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

## External dependencies:
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

Main dialog with the configuration options showing a preview of the captured area (whole 3840x1080 desktop) and the current position of the overlayed images (both camera and pomodoro statistics).

![Main dialog]()

Once started the tray icon will provide the information on the progression of the timer and the session.

![Tray icons]()

If the pomodoro timer is being used the statistics can be shown and the current task can be changed while in a session. 

![Pomodoro statistics]()