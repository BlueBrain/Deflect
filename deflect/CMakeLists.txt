
# Copyright (c) 2013-2016, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(DEFLECT_PUBLIC_HEADERS
  config.h
  Event.h
  EventReceiver.h
  FrameDispatcher.h
  Frame.h
  ImageWrapper.h
  MTQueue.h
  Segment.h
  SegmentParameters.h
  Server.h
  SizeHints.h
  Stream.h
  types.h
)

set(DEFLECT_HEADERS
  ImageSegmenter.h
  MessageHeader.h
  NetworkProtocol.h
  ReceiveBuffer.h
  ServerWorker.h
  Socket.h
  StreamPrivate.h
)

set(DEFLECT_SOURCES
  Event.cpp
  Frame.cpp
  FrameDispatcher.cpp
  ImageSegmenter.cpp
  ImageWrapper.cpp
  MessageHeader.cpp
  MetaTypeRegistration.cpp
  ReceiveBuffer.cpp
  Server.cpp
  ServerWorker.cpp
  Socket.cpp
  Stream.cpp
  StreamPrivate.cpp
  StreamSendWorker.cpp
)

set(DEFLECT_LINK_LIBRARIES
  PUBLIC ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY}
  PRIVATE Qt5::Concurrent Qt5::Core Qt5::Network
)
set(DEFLECT_PUBLIC_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

if(DEFLECT_USE_LIBJPEGTURBO)
  list(APPEND DEFLECT_PUBLIC_HEADERS
    SegmentDecoder.h
  )
  list(APPEND DEFLECT_HEADERS
    ImageJpegCompressor.h
    ImageJpegDecompressor.h
  )
  list(APPEND DEFLECT_SOURCES
    ImageJpegCompressor.cpp
    ImageJpegDecompressor.cpp
    SegmentDecoder.cpp
  )
  list(APPEND DEFLECT_LINK_LIBRARIES ${LibJpegTurbo_LIBRARIES})
endif()

if(DEFLECT_USE_SERVUS)
  list(APPEND DEFLECT_LINK_LIBRARIES Servus)
endif()

common_library(Deflect)

if(Qt5Qml_FOUND AND Qt5Quick_FOUND AND NOT Qt5Quick_VERSION VERSION_LESS 5.4)
  add_subdirectory(qt)
endif()
