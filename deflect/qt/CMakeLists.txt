
# Copyright (c) 2015, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(DEFLECTQT_HEADERS EventReceiver.h QmlStreamerImpl.h)
set(DEFLECTQT_PUBLIC_HEADERS QmlStreamer.h)
set(DEFLECTQT_SOURCES EventReceiver.cpp QmlStreamer.cpp QmlStreamerImpl.cpp)
set(DEFLECTQT_LINK_LIBRARIES
  PUBLIC Deflect Qt5::Quick PRIVATE Qt5::Qml)
set(DEFLECTQT_INCLUDE_NAME deflect/qt)
set(DEFLECTQT_NAMESPACE deflectqt)
common_library(DeflectQt)
