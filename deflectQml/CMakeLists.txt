
# Copyright (c) 2015, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(DEFLECTQML_MOC_HEADERS EventReceiver.h QmlStreamerImpl.h)
set(DEFLECTQML_PUBLIC_HEADERS QmlStreamer.h)
set(DEFLECTQML_SOURCES EventReceiver.cpp QmlStreamer.cpp QmlStreamerImpl.cpp)
set(DEFLECTQML_LINK_LIBRARIES
  PUBLIC Deflect Qt5::Quick PRIVATE Qt5::Qml)

common_library(DeflectQml)
