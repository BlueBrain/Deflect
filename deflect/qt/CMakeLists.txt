
# Copyright (c) 2015-2016, EPFL/Blue Brain Project
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>

set(DEFLECTQT_HEADERS
  EventReceiver.h
  QmlGestures.h
  QmlStreamerImpl.h
)
set(DEFLECTQT_PUBLIC_HEADERS
  QmlStreamer.h
  TouchInjector.h
)
set(DEFLECTQT_SOURCES
  EventReceiver.cpp
  QmlStreamer.cpp
  QmlStreamerImpl.cpp
  TouchInjector.cpp
)
set(DEFLECTQT_LINK_LIBRARIES
  PUBLIC Deflect Qt5::Quick PRIVATE Qt5::Qml)
set(DEFLECTQT_INCLUDE_NAME deflect/qt)
set(DEFLECTQT_NAMESPACE deflectqt)
common_library(DeflectQt)

if(TARGET Qt5::Gui)
  target_include_directories(DeflectQt SYSTEM PRIVATE
    "$<BUILD_INTERFACE:${Qt5Gui_PRIVATE_INCLUDE_DIRS}>")
endif()
