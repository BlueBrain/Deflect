
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(DEFLECT_PUBLIC_HEADERS
  ${COMMON_INCLUDES}
  Event.h
  ImageJpegCompressor.h
  ImageSegmenter.h
  ImageWrapper.h
  MessageHeader.h
  NetworkProtocol.h
  PixelStreamSegment.h
  PixelStreamSegmentParameters.h
  Socket.h
  Stream.h
  types.h
)

set(DEFLECT_MOC_HEADERS
  Socket.h
  StreamPrivate.h
)

qt4_wrap_cpp(MOC_OUTFILES ${DEFLECT_MOC_HEADERS})

set(DEFLECT_SOURCES
  ${MOC_OUTFILES}
  Event.cpp
  ImageJpegCompressor.cpp
  ImageSegmenter.cpp
  ImageWrapper.cpp
  MessageHeader.cpp
  Socket.cpp
  Stream.cpp
  StreamPrivate.cpp
  StreamSendWorker.cpp
)

set(DEFLECT_LINK_LIBRARIES
  ${QT_QTCORE_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${LibJpegTurbo_LIBRARIES}
  ${Boost_THREAD_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
)

common_library(Deflect)
