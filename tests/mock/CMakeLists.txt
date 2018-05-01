
# Copyright (c) 2013-2018, EPFL/Blue Brain Project
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>
#
# Generates a mock library used by the cpp unit tests.

set(DEFLECTMOCK_HEADERS
  boost_test_thread_safe.h
  DeflectServer.h
  FrameUtils.h
  MinimalGlobalQtApp.h
  MinimalDeflectServer.h
  MockServer.h
  Timer.h
)

set(DEFLECTMOCK_SOURCES
  DeflectServer.cpp
  MinimalDeflectServer.cpp
  MockServer.cpp
)

set(DEFLECTMOCK_LINK_LIBRARIES Deflect ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  Qt5::Core Qt5::Network)

set(DEFLECTMOCK_INCLUDE_NAME deflect/mock)
set(DEFLECTMOCK_OMIT_LIBRARY_HEADER ON)
set(DEFLECTMOCK_OMIT_VERSION_HEADERS ON)
set(DEFLECTMOCK_OMIT_CHECK_TARGETS ON)
set(DEFLECTMOCK_OMIT_INSTALL ON)
common_library(DeflectMock)
