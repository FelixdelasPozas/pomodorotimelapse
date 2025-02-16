cmake_minimum_required (VERSION 3.10.0)

project (DesktopCapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Version Number
set (DESKTOPCAPTURE_VERSION_MAJOR 1)
set (DESKTOPCAPTURE_VERSION_MINOR 2)
set (DESKTOPCAPTURE_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

SET(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)

# Find the QtWidgets library
find_package(Qt6 COMPONENTS Widgets Multimedia)

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
find_package(dlib REQUIRED)
find_package(TBB REQUIRED)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  )

set(CMAKE_CXX_FLAGS " -Wall -Wno-deprecated -std=c++17 -mwindows -flarge-source-files")

qt6_add_resources(RESOURCES
	rsc/resources.qrc
  rsc/resources_face.qrc
	)

qt6_wrap_ui(CORE_UI
  MainWindow.ui
  ProbeResolutionsDialog.ui
  PomodoroStatistics.ui
  AboutDialog.ui
  )
	
set (CORE_SOURCES
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

set(CORE_EXTERNAL_LIBS
  ${QT_LIBRARIES}
  ${OpenCV_LIBS}
  Qt6::Widgets
  Qt6::Multimedia
  pthread
  dlib::dlib
  libvpx
  libyuv
  TBB::tbb
)

add_executable(DesktopCapture ${CORE_SOURCES})
target_link_libraries (DesktopCapture ${CORE_EXTERNAL_LIBS})
