project (DesktopCapture)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (DESKTOPCAPTURE_VERSION_MAJOR 0)
set (DESKTOPCAPTURE_VERSION_MINOR 1)
set (DESKTOPCAPTURE_VERSION_PATCH 0)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if(DEFINED DESKTOPCAPTURE_VERSION_MAJOR)
  configure_file("${PROJECT_SOURCE_DIR}/DesktopCapture.rc.in" "${PROJECT_BINARY_DIR}/DesktopCapture.rc")
  set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/DesktopCapture.rc)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  enable_language(RC)
endif(DEFINED DESKTOPCAPTURE_VERSION_MAJOR)

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

find_package(OpenCV REQUIRED)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/Camera
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  )

set(CMAKE_CXX_FLAGS " -Wall -Wno-deprecated -std=c++11 -fpermissive -mwindows")

# Add Qt Resource files
qt4_add_resources(RESOURCES
	rsc/resources.qrc
	)

qt4_wrap_ui(CORE_UI
  # .ui para Qt
  MainWindow.ui
  ProbeResolutionsDialog.ui
  
  #Camera/CameraConnectDialog.ui
  #Camera/CameraView.ui
  #Camera/ImageProcessingSettingsDialog.ui
  #Camera/MainWindow.ui
  )
	
qt4_wrap_cpp(CORE_MOC
  # .h para Qt
  DesktopCapture.h
  ProbeResolutionsDialog.h
  CaptureDesktopThread.h
  Pomodoro.h
  
  #Camera/CameraConnectDialog.h
  #Camera/CameraView.h
  #Camera/CaptureThread.h
  #Camera/FrameLabel.h
  #Camera/ImageProcessingSettingsDialog.h
  #Camera/MainWindow.h
  #Camera/ProcessingThread.h
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
  
  #Camera/CameraConnectDialog.cpp
  #Camera/CameraView.cpp
  #Camera/CaptureThread.cpp
  #Camera/FrameLabel.cpp
  #Camera/ImageProcessingSettingsDialog.cpp
  #Camera/main.cpp
  #Camera/MainWindow.cpp
  #Camera/MatToQImage.cpp
  #Camera/ProcessingThread.cpp
  #Camera/SharedImageBuffer.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/DesktopCapture.rc
  )

 
set(CORE_EXTERNAL_LIBS
  ${QT_LIBRARIES}
  ${OpenCV_LIBS}
)

add_executable(DesktopCapture ${CORE_SOURCES})
target_link_libraries (DesktopCapture ${CORE_EXTERNAL_LIBS})