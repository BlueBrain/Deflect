
# Copyright (c) 2013-2016, EPFL/Blue Brain Project
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Deflect VERSION 0.11.0)
set(Deflect_VERSION_ABI 4)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
                              ${CMAKE_SOURCE_DIR}/CMake/common)
include(GitExternal)

set(DEFLECT_DESCRIPTION "A fast C++ library for streaming pixels and events")
set(DEFLECT_MAINTAINER "Blue Brain Project <bbp-open-source@googlegroups.com>")
set(DEFLECT_LICENSE BSD)
set(DEFLECT_DEPENDENT_LIBRARIES Boost)
set(DEFLECT_DEB_DEPENDS libjpeg-turbo8-dev libturbojpeg freeglut3-dev
  libboost-test-dev libboost-date-time-dev libboost-program-options-dev
  libboost-serialization-dev libboost-system-dev libboost-thread-dev
  qtbase5-dev)
set(DEFLECT_PORT_DEPENDS boost freeglut qt5)

include(Common)

common_find_package(Boost REQUIRED COMPONENTS date_time program_options
                                unit_test_framework serialization system thread)
common_find_package(GLUT)
common_find_package(LibJpegTurbo REQUIRED)
common_find_package(OpenGL)
common_find_package(Qt5Concurrent REQUIRED SYSTEM)
common_find_package(Qt5Core REQUIRED)
if(APPLE)
  common_find_package(Qt5MacExtras)
endif()
common_find_package(Qt5Network REQUIRED)
common_find_package(Qt5Qml)
common_find_package(Qt5Quick)
common_find_package(Qt5Widgets REQUIRED)
common_find_package(Servus)
common_find_package_post()

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
