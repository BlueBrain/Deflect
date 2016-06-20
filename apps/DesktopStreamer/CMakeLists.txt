
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>

set(DESKTOPSTREAMER_HEADERS
  MainWindow.h
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

if(DEFLECT_USE_SERVUS)
  list(APPEND DESKTOPSTREAMER_LINK_LIBRARIES Servus)
endif()

set(DESKTOPSTREAMER_APP_NAME desktopstreamer)

if(APPLE)
  set(DESKTOPSTREAMER_APP_NAME DesktopStreamer)
  string(TIMESTAMP _CURRENT_YEAR "%Y")
  set(DESKTOPSTREAMER_COPYRIGHT "EPFL/Blue Brain Project ${_CURRENT_YEAR}")
  set(DESKTOPSTREAMER_ICON desktopstreamer.icns)

  list(APPEND DESKTOPSTREAMER_SOURCES AppNapSuspender.mm)
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
elseif(MSVC)
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
