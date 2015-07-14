
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

set(IGNORE_LIB_TARGETS Mock)

set(MOCK_INCLUDE_NAME mock)
set(MOCK_LINK_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Network_LIBRARIES})
common_library(Mock)

