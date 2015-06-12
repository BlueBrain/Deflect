
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(DEFLECT_PUBLIC_HEADERS
  ${COMMON_INCLUDES}
  AbstractCommandHandler.h
  config.h
  Command.h
  CommandType.h
  Event.h
  Frame.h
  ImageWrapper.h
  ReceiveBuffer.h
  Segment.h
  SegmentParameters.h
  Stream.h
  types.h
)

set(DEFLECT_MOC_PUBLIC_HEADERS
  CommandHandler.h
  EventReceiver.h
  FrameDispatcher.h
  Server.h
)

set(DEFLECT_HEADERS
  ImageJpegCompressor.h
  ImageJpegDecompressor.h
  ImageSegmenter.h
  MessageHeader.h
  NetworkProtocol.h
)

set(DEFLECT_MOC_HEADERS
  Socket.h
  StreamPrivate.h
  ServerWorker.h
)

set(DEFLECT_SOURCES
  Command.cpp
  CommandHandler.cpp
  CommandType.cpp
  Event.cpp
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
  ${Qt5Concurrent_LIBRARIES}
  ${Qt5Core_LIBRARIES}
  ${Qt5Network_LIBRARIES}
  ${Boost_THREAD_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
)

if(DEFLECT_USE_LIBJPEGTURBO)
  list(APPEND DEFLECT_PUBLIC_HEADERS
    SegmentDecoder.h
  )
  list(APPEND DEFLECT_SOURCES
    ImageJpegCompressor.cpp
    ImageJpegDecompressor.cpp
    SegmentDecoder.cpp
  )
  list(APPEND DEFLECT_LINK_LIBRARIES ${LibJpegTurbo_LIBRARIES})
endif()

if(DEFLECT_USE_SERVUS)
  list(APPEND DEFLECT_LINK_LIBRARIES ${SERVUS_LIBRARIES})
endif()

common_library(Deflect)
