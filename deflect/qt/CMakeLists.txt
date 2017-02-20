
# Copyright (c) 2015-2016, EPFL/Blue Brain Project
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>

set(DEFLECTQT_HEADERS
  EventReceiver.h
  helpers.h
  QmlGestures.h
  QmlStreamerImpl.h
)
set(DEFLECTQT_PUBLIC_HEADERS
  OffscreenQuickView.h
  QmlStreamer.h
  QuickRenderer.h
  TouchInjector.h
)
set(DEFLECTQT_SOURCES
  EventReceiver.cpp
  OffscreenQuickView.cpp
  QmlStreamer.cpp
  QmlStreamerImpl.cpp
  QuickRenderer.cpp
  TouchInjector.cpp

)
set(DEFLECTQT_LINK_LIBRARIES
  PUBLIC Deflect Qt5::Quick PRIVATE Qt5::Qml)
set(DEFLECTQT_INCLUDE_NAME deflect/qt)
set(DEFLECTQT_NAMESPACE deflectqt)
common_library(DeflectQt)

if(DEFLECT_QMLSTREAMER_MULTITHREADED)
  target_compile_definitions(DeflectQt PRIVATE DEFLECTQT_MULTITHREADED)
endif()
