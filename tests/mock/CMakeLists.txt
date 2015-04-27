
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#
# Generates a mock library used by the cpp unit tests.

set(MOCK_HEADERS
  MinimalGlobalQtApp.h
)

set(MOCK_MOC_HEADERS
  MockNetworkListener.h
)

set(MOCK_SOURCES
  MockNetworkListener.cpp
)

qt5_wrap_cpp(MOC_OUTFILES ${MOCK_MOC_HEADERS})

set(IGNORE_LIB_TARGETS Mock)
add_library(Mock SHARED ${MOC_OUTFILES} ${MOCK_HEADERS} ${MOCK_SOURCES})
set_target_properties(Mock PROPERTIES
  COMPILE_DEFINITIONS "DEFLECT_SHARED")
target_link_libraries(Mock ${Qt5Core_LIBRARIES} ${Qt5Network_LIBRARIES})
