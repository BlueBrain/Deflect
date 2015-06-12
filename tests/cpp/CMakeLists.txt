
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>

set(TEST_LIBRARIES Deflect Mock ${Boost_LIBRARIES})
if(NOT DEFLECT_USE_LIBJPEGTURBO)
  set(EXCLUDE_FROM_TESTS SegmentDecoderTests.cpp)
endif()
include(CommonCTest)
