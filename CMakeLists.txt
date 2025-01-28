project (DesktopCapture)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (DESKTOPCAPTURE_VERSION_MAJOR 1)
set (DESKTOPCAPTURE_VERSION_MINOR 2)
set (DESKTOPCAPTURE_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Widgets Multimedia)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if(DEFINED MINGW)
  configure_file("${PROJECT_SOURCE_DIR}/DesktopCapture.rc.in" "${PROJECT_BINARY_DIR}/DesktopCapture.rc")
  configure_file("${PROJECT_SOURCE_DIR}/installer/script.iss.in" "${PROJECT_BINARY_DIR}/script.iss")
  set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/DesktopCapture.rc)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  enable_language(RC)
endif(DEFINED MINGW)

find_package(OpenCV REQUIRED)

set(LIBVPX_LIBRARY_DIR "D:/Desarrollo/Bibliotecas/libvpx")
set(LIBYUV_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/libyuv/include")

set(DLIB_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/dlib-19.21")
set(DLIB_LIBRARY "D:/Desarrollo/Bibliotecas/dlib-build/dlib/libdlib.a")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${LIBVPX_LIBRARY_DIR}
  ${LIBYUV_INCLUDE_DIR}
  ${DLIB_INCLUDE_DIR}
  )

set(CMAKE_CXX_FLAGS " -Wall -Wno-deprecated -std=c++11 -fpermissive -mwindows")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

# Add Qt Resource files
qt5_add_resources(RESOURCES
	rsc/resources.qrc
	)

qt5_wrap_ui(CORE_UI
  # .ui para Qt
  MainWindow.ui
  ProbeResolutionsDialog.ui
  PomodoroStatistics.ui
  AboutDialog.ui
  )
	
set (CORE_SOURCES
  # ficheros para compilar
  ${RESOURCES}
  ${CORE_MOC}
  ${CORE_UI}
  Main.cpp
  DesktopCapture.cpp
  ProbeResolutionsDialog.cpp
  CaptureDesktopThread.cpp
  Pomodoro.cpp
  PomodoroStatistics.cpp
  AboutDialog.cpp
  VPXInterface.cpp
  webmEBMLwriter.cpp
  Utils.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/DesktopCapture.rc
)

SET_SOURCE_FILES_PROPERTIES(${CORE_SOURCES} PROPERTIES OBJECT_DEPENDS "${CORE_UI}")

set(VPX_LIBS
  D:/Desarrollo/Bibliotecas/libvpx/libvpx.a
)

set(YUV_LIBS
  D:/Desarrollo/Bibliotecas/libyuv-build/libyuv.a
)

set(CORE_EXTERNAL_LIBS
  ${QT_LIBRARIES}
  ${OpenCV_LIBS}
  ${VPX_LIBS}
  ${YUV_LIBS}
  ${DLIB_LIBRARY}
  Qt5::Widgets
  Qt5::Multimedia
  pthread
)

add_executable(DesktopCapture ${CORE_SOURCES})
target_link_libraries (DesktopCapture ${CORE_EXTERNAL_LIBS})
qt5_use_modules(DesktopCapture Widgets Multimedia)