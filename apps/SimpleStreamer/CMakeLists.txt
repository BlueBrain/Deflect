
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>

set(SIMPLESTREAMER_SOURCES main.cpp)
set(SIMPLESTREAMER_LINK_LIBRARIES Deflect ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

add_executable(simplestreamer ${SIMPLESTREAMER_SOURCES})
target_link_libraries(simplestreamer ${SIMPLESTREAMER_LINK_LIBRARIES})

INSTALL(TARGETS simplestreamer RUNTIME DESTINATION bin COMPONENT simplestreamer)
