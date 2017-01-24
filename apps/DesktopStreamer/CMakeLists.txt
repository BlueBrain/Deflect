
# Copyright (c) 2013-2017, EPFL/Blue Brain Project
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>

set(DESKTOPSTREAMER_HEADERS
  MainWindow.h
  nameUtils.h
  Stream.h
)

set(DESKTOPSTREAMER_SOURCES
  main.cpp
  MainWindow.cpp
  MainWindow.ui
  resources.qrc
  Stream.cpp
)

set(DESKTOPSTREAMER_LINK_LIBRARIES
  Deflect
  Qt5::Core
  Qt5::Network
  Qt5::Widgets
)

set(DEFLECT_DESKTOPSTREAMER_HOSTS
    "\
{\"DisplayWall Ground floor\", \"bbpav02.epfl.ch\"}, \
{\"DisplayWall 3rd floor\", \"bbpav04.epfl.ch\"}, \
{\"DisplayWall 5th floor\", \"bbpav05.epfl.ch\"}, \
{\"DisplayWall 6th floor\", \"bbpav06.epfl.ch\"}"
    CACHE STRING "List of default hosts for the DesktopStreamer application"
)
set_source_files_properties(MainWindow.cpp PROPERTIES COMPILE_DEFINITIONS
  _HOSTS=${DEFLECT_DESKTOPSTREAMER_HOSTS}
)

set(DESKTOPSTREAMER_APP_NAME desktopstreamer)

if(APPLE)
  set(DESKTOPSTREAMER_APP_NAME DesktopStreamer)
  string(TIMESTAMP _CURRENT_YEAR "%Y")
  set(DESKTOPSTREAMER_COPYRIGHT "EPFL/Blue Brain Project ${_CURRENT_YEAR}")
  set(DESKTOPSTREAMER_ICON desktopstreamer.icns)

  list(APPEND DESKTOPSTREAMER_SOURCES nameUtils.mm)
  list(APPEND DESKTOPSTREAMER_LINK_LIBRARIES "-framework Foundation")

  if(TARGET Qt5::MacExtras)
    list(APPEND DESKTOPSTREAMER_HEADERS DesktopWindowsModel.h)
    list(APPEND DESKTOPSTREAMER_SOURCES DesktopWindowsModel.mm)
    list(APPEND DESKTOPSTREAMER_LINK_LIBRARIES
      Qt5::MacExtras "-framework AppKit"
    )
  endif()
  if(OSX_VERSION VERSION_LESS 10.9)
    list(APPEND DESKTOPSTREAMER_LINK_LIBRARIES "-framework ApplicationServices")
  else()
    list(APPEND DESKTOPSTREAMER_LINK_LIBRARIES "-framework CoreGraphics")
  endif()
else()
  list(APPEND DESKTOPSTREAMER_SOURCES nameUtils.cpp)
endif()

if(MSVC)
  list(APPEND DESKTOPSTREAMER_LINK_LIBRARIES Ws2_32)
endif()

common_application(${DESKTOPSTREAMER_APP_NAME} GUI)

if(APPLE)
  # create a zip for Puppet deployment
  install(CODE "execute_process(COMMAND zip -r
    ${DESKTOPSTREAMER_APP_NAME}-${PROJECT_VERSION}.zip ${DESKTOPSTREAMER_APP_NAME}.app
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)" COMPONENT apps)
endif()

set(CPACK_PACKAGE_LICENSE "LGPL")
include(CPackConfig)
