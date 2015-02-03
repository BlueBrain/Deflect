
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(DEFLECT_PUBLIC_HEADERS
  ${COMMON_INCLUDES}
  AbstractCommandHandler.h
  config.h
  Command.h
  CommandHandler.h
  CommandType.h
  Event.h
  EventReceiver.h
  ImageWrapper.h
  NetworkListener.h
  PixelStreamBuffer.h
  PixelStreamFrame.h
  PixelStreamDispatcher.h
  PixelStreamSegment.h
  PixelStreamSegmentParameters.h
  Socket.h
  Stream.h
  types.h
)

set(DEFLECT_HEADERS
  ${COMMON_INCLUDES}
  ImageJpegCompressor.h
  ImageJpegDecompressor.h
  ImageSegmenter.h
  MessageHeader.h
  NetworkListenerWorker.h
  NetworkProtocol.h
  PixelStreamFrame.h
)

set(DEFLECT_MOC_HEADERS
  CommandHandler.h
  EventReceiver.h
  NetworkListener.h
  NetworkListenerWorker.h
  PixelStreamDispatcher.h
  Socket.h
  StreamPrivate.h
)

qt4_wrap_cpp(MOC_OUTFILES ${DEFLECT_MOC_HEADERS})

set(DEFLECT_SOURCES
  ${MOC_OUTFILES}
  Command.cpp
  CommandHandler.cpp
  CommandType.cpp
  Event.cpp
  ImageSegmenter.cpp
  ImageWrapper.cpp
  MessageHeader.cpp
  MetaTypeRegistration.cpp
  NetworkListener.cpp
  NetworkListenerWorker.cpp
  PixelStreamBuffer.cpp
  PixelStreamDispatcher.cpp
  Socket.cpp
  Stream.cpp
  StreamPrivate.cpp
  StreamSendWorker.cpp
)

set(DEFLECT_LINK_LIBRARIES
  ${QT_QTCORE_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
)

if(DEFLECT_USE_LIBJPEGTURBO)
  list(APPEND DEFLECT_PUBLIC_HEADERS PixelStreamSegmentDecoder.h)
  list(APPEND DEFLECT_SOURCES
    ImageJpegCompressor.cpp
    ImageJpegDecompressor.cpp
    PixelStreamSegmentDecoder.cpp
  )
  list(APPEND DEFLECT_LINK_LIBRARIES ${LibJpegTurbo_LIBRARIES})
endif()

common_library(Deflect)
