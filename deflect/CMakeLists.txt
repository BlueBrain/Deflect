
# Copyright (c) 2013-2017, EPFL/Blue Brain Project
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(DEFLECT_PUBLIC_HEADERS
  config.h
  Event.h
  EventReceiver.h
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
  FrameDispatcher.h
  ImageSegmenter.h
  MessageHeader.h
  NetworkProtocol.h
  ReceiveBuffer.h
  ServerWorker.h
  Socket.h
  SourceBuffer.h
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
  SourceBuffer.cpp
  Stream.cpp
  StreamPrivate.cpp
  StreamSendWorker.cpp
)

set(DEFLECT_LINK_LIBRARIES PRIVATE Qt5::Concurrent Qt5::Core Qt5::Network)

if(APPLE)
  list(APPEND DEFLECT_PUBLIC_HEADERS AppNapSuspender.h)
  list(APPEND DEFLECT_SOURCES AppNapSuspender.mm)
  list(APPEND DEFLECT_LINK_LIBRARIES PRIVATE "-framework Foundation")
endif()

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

common_library(Deflect)

if(Qt5Qml_FOUND AND Qt5Quick_FOUND AND NOT Qt5Quick_VERSION VERSION_LESS 5.4)
  add_subdirectory(qt)
endif()
