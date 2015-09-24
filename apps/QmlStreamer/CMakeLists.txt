
# Copyright (c) 2015, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>

set(QMLSTREAMER_SOURCES main.cpp)
set(QMLSTREAMER_RESOURCES resources.qrc)
set(QMLSTREAMER_LINK_LIBRARIES DeflectQml)
common_application(QmlStreamer)
