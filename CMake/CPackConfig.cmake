# Copyright (c) BBP/EPFL 2011-2015
# Stefan.Eilemann@epfl.ch
# All rights reserved. Do not distribute without further notice.

# General CPack configuration
# Info: http://www.itk.org/Wiki/CMake:Component_Install_With_CPack

set(CPACK_PACKAGE_VENDOR "bluebrain.epfl.ch")
set(CPACK_PACKAGE_EXECUTABLES "${DESKTOPSTREAMER_APP_NAME}")

# Linux Debian specific settings
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qtbase5-dev, libturbojpeg" )
set(CPACK_DEB_COMPONENT_INSTALL ON) # Set this to package only components in CPACK_COMPONENTS_ALL
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1) # Don't make a separate package for each component

include(CommonCPack)
