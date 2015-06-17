Desktop Capture
===============
DesktopCapture is a tool to create a video or a serie of images of the desktop activity, taken at given intervals. It's similar to many ''chronolapse'' tools available on the internet but with a few quirks.

Optionally:
* the region of the desktop to be captured can be defined as the whole desktop or a single monitor.
* if a webcam is detected the image can overlayed over the desktop captured image (and just for fun several masks can be put over the face detected in the camera).
* implements the pomodoro technique and the statistics from the session can be overlayed over the desktop captured image. 
* the output can be scaled.
* the overlayed pictures con be configured in size, position and composition mode. 

# Output file formats
The tool outputs the following files:
* **Video format** : Google's webm video.
* **Image format** : a serie of PNG files. 

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows or [gcc](http://gcc.gnu.org/) on Linux.

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
![Main dialog](https://cloud.githubusercontent.com/assets/12167134/7867872/e2fd4c28-0578-11e5-93bb-56c7ee8b26df.jpg)
