
# Copyright (c) 2018, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>

set(DEFLECTSERVER_PUBLIC_HEADERS
  EventReceiver.h
  Frame.h
  Server.h
)
set(DEFLECTSERVER_HEADERS
  FrameDispatcher.h
  ServerWorker.h
  ReceiveBuffer.h
  SourceBuffer.h
)
set(DEFLECTSERVER_SOURCES
  Frame.cpp
  FrameDispatcher.cpp
  Server.cpp
  ServerWorker.cpp
  ReceiveBuffer.cpp
  SourceBuffer.cpp
)

set(DEFLECTSERVER_LINK_LIBRARIES
  PUBLIC Deflect Qt5::Core Qt5::Network
)

if(DEFLECT_USE_LIBJPEGTURBO)
  list(APPEND DEFLECTSERVER_PUBLIC_HEADERS
    SegmentDecoder.h
  )
  list(APPEND DEFLECTSERVER_HEADERS
    ImageJpegDecompressor.h
  )
  list(APPEND DEFLECTSERVER_SOURCES
    ImageJpegDecompressor.cpp
    SegmentDecoder.cpp
  )
  list(APPEND DEFLECTSERVER_LINK_LIBRARIES PRIVATE ${LibJpegTurbo_LIBRARIES})
endif()

set(DEFLECTSERVER_INCLUDE_NAME deflect/server)
set(DEFLECTSERVER_OMIT_VERSION_HEADERS ON)
# avoid conflict between server.h and Server.h on case-insensitive file systems
set(DEFLECTSERVER_OMIT_LIBRARY_HEADER ON)

common_library(DeflectServer)
