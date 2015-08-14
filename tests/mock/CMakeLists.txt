
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#
# Generates a mock library used by the cpp unit tests.

set(MOCK_HEADERS
  MinimalGlobalQtApp.h
)

set(MOCK_MOC_HEADERS
  MockServer.h
)

set(MOCK_SOURCES
  MockServer.cpp
)

set(MOCK_INCLUDE_NAME mock)
set(MOCK_NAMESPACE mock)
set(MOCK_LINK_LIBRARIES Qt5::Core Qt5::Network)
common_library(Mock)

