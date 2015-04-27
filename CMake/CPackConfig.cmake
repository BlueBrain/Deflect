# Copyright (c) BBP/EPFL 2011-2015
# Stefan.Eilemann@epfl.ch
# All rights reserved. Do not distribute without further notice.

# General CPack configuration
# Info: http://www.itk.org/Wiki/CMake:Component_Install_With_CPack

set(CPACK_PACKAGE_EXECUTABLES "${DESKTOPSTREAMER_APP_NAME}")
set(CPACK_PACKAGE_NAME "${DESKTOPSTREAMER_APP_NAME}")
set(CPACK_PACKAGE_CONTACT "Daniel Nachbaur <daniel.nachbaur@epfl.ch>")
set(CPACK_PACKAGE_VENDOR "https://github.com/BlueBrain/Deflect")

set(CPACK_COMPONENT_DESKTOPSTREAMER_DISPLAY_NAME "DesktopStreamer Application")
set(CPACK_COMPONENT_DESKTOPSTREAMER_DESCRIPTION "DesktopStreamer is an application that lets you stream your desktop to a running DisplayCluster instance.")
set(CPACK_COMPONENT_DESKTOPSTREAMER_DEPENDS Deflect)

# Currently we only package desktopstreamer and the Deflect library
set(CPACK_COMPONENTS_ALL desktopstreamer lib)

# Linux Debian specific settings
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qtbase5-dev, libturbojpeg" )
set(CPACK_DEB_COMPONENT_INSTALL ON) # Set this to package only components in CPACK_COMPONENTS_ALL
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1) # Don't make a separate package for each component

include(CommonCPack)
