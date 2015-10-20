
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Deflect)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
                              ${CMAKE_SOURCE_DIR}/CMake/common)
include(GitExternal)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "8")
set(VERSION_PATCH "0")
set(VERSION_ABI 2)

set(DEFLECT_DESCRIPTION "A fast C++ library for streaming pixels and events")
set(DEFLECT_MAINTAINER "Blue Brain Project <bbp-open-source@googlegroups.com>")
set(DEFLECT_LICENSE BSD)
set(DEFLECT_DEPENDENT_LIBRARIES Boost)

include(Common)

common_package(Boost REQUIRED COMPONENTS date_time program_options
                                unit_test_framework serialization system thread)
common_package(GLUT)
common_package(LibJpegTurbo REQUIRED)
common_package(OpenGL)
common_package(Qt5Concurrent REQUIRED SYSTEM)
common_package(Qt5Core REQUIRED)
common_package(Qt5Network REQUIRED)
common_package(Qt5Qml)
common_package(Qt5Quick)
common_package(Qt5Widgets REQUIRED)
common_package(Servus)
common_package_post()

add_subdirectory(deflect)
add_subdirectory(apps)
add_subdirectory(tests)
add_subdirectory(doc)

set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
set(DOXYGEN_MAINPAGE_MD README.md)
set(DOXYGEN_EXTRA_INPUT ${PROJECT_SOURCE_DIR}/README.md)
include(DoxygenRule) # must be after all targets

set(CTEST_CUSTOM_WARNING_EXCEPTION ${CTEST_CUSTOM_WARNING_EXCEPTION}
  # http://public.kitware.com/Bug/view.php?id=10179
  "ld: warning: duplicate dylib")
include(CTest)
