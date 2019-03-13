
# Copyright (c) 2013-2018, EPFL/Blue Brain Project
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(DEFLECT_PUBLIC_HEADERS
  Event.h
  ImageWrapper.h
  Observer.h
  SizeHints.h
  Stream.h
  types.h
)

set(DEFLECT_HEADERS
  moodycamel/blockingconcurrentqueue.h
  moodycamel/concurrentqueue.h
  ImageSegmenter.h
  MessageHeader.h
  MTQueue.h
  NetworkProtocol.h
  Segment.h
  SegmentParameters.h
  Socket.h
  StreamPrivate.h
  TaskBuilder.h
)

set(DEFLECT_SOURCES
  Event.cpp
  ImageSegmenter.cpp
  ImageWrapper.cpp
  MessageHeader.cpp
  MetaTypeRegistration.cpp
  Observer.cpp
  Socket.cpp
  Stream.cpp
  StreamPrivate.cpp
  StreamSendWorker.cpp
  TaskBuilder.cpp
)

set(DEFLECT_LINK_LIBRARIES PRIVATE Qt5::Concurrent Qt5::Core Qt5::Network)

if(APPLE)
  list(APPEND DEFLECT_PUBLIC_HEADERS AppNapSuspender.h)
  list(APPEND DEFLECT_SOURCES AppNapSuspender.mm)
  list(APPEND DEFLECT_LINK_LIBRARIES PRIVATE "-framework Foundation")
endif()

if(DEFLECT_USE_LIBJPEGTURBO)
  list(APPEND DEFLECT_HEADERS
    ImageJpegCompressor.h
  )
  list(APPEND DEFLECT_SOURCES
    ImageJpegCompressor.cpp
  )
  list(APPEND DEFLECT_LINK_LIBRARIES PRIVATE ${LibJpegTurbo_LIBRARIES})
endif()

common_library(Deflect)

add_subdirectory(server)

if(Qt5Qml_FOUND AND Qt5Quick_FOUND AND QtOpenGL_FOUND AND
   NOT Qt5Quick_VERSION VERSION_LESS 5.4)
  add_subdirectory(qt)
endif()
