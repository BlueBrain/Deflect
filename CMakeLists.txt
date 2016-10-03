
# Copyright (c) 2013-2016, EPFL/Blue Brain Project
#                          Raphael Dumusc <raphael.dumusc@epfl.ch>
#                          Daniel Nachbaur <daniel.nachbaur@epfl.ch>

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Deflect VERSION 0.12.0)
set(Deflect_VERSION_ABI 5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
                              ${CMAKE_SOURCE_DIR}/CMake/common)
include(GitExternal)

set(DEFLECT_DESCRIPTION "A fast C++ library for streaming pixels and events")
set(DEFLECT_MAINTAINER "Blue Brain Project <bbp-open-source@googlegroups.com>")
set(DEFLECT_LICENSE BSD)
set(DEFLECT_DEB_DEPENDS freeglut3-dev libxi-dev libxmu-dev
  libjpeg-turbo8-dev libturbojpeg
  libboost-program-options-dev libboost-test-dev
  qtbase5-dev qtbase5-private-dev qtdeclarative5-dev)
set(DEFLECT_PORT_DEPENDS boost freeglut qt5)

include(Common)

common_find_package(Boost COMPONENTS program_options unit_test_framework)
common_find_package(GLUT)
common_find_package(LibJpegTurbo REQUIRED)
common_find_package(OpenGL)
common_find_package(Qt5Concurrent REQUIRED SYSTEM)
common_find_package(Qt5Core REQUIRED)
common_find_package(Qt5Gui COMPONENTS Private) # For Qml WebEngineView
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
if(Boost_FOUND)
  add_subdirectory(tests)
endif()
add_subdirectory(doc)

set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
set(DOXYGEN_MAINPAGE_MD README.md)
set(DOXYGEN_EXTRA_INPUT ${PROJECT_SOURCE_DIR}/README.md)
include(DoxygenRule) # must be after all targets

set(CTEST_CUSTOM_WARNING_EXCEPTION ${CTEST_CUSTOM_WARNING_EXCEPTION}
  # http://public.kitware.com/Bug/view.php?id=10179
  "ld: warning: duplicate dylib")
include(CTest)
