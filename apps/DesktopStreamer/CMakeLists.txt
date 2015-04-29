
# Copyright (c) 2013-2015, EPFL/Blue Brain Project
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>

set(DESKTOPSTREAMER_MOC_HEADERS
  MainWindow.h
  DesktopSelectionWindow.h
  DesktopSelectionRectangle.h
)

set(DESKTOPSTREAMER_SOURCES
  DesktopSelectionRectangle.cpp
  DesktopSelectionWindow.cpp
  DesktopSelectionView.cpp
  main.cpp
  MainWindow.cpp
)

set(DESKTOPSTREAMER_LINK_LIBRARIES Deflect ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})

set(DESKTOPSTREAMER_RESOURCES resources.qrc)

qt4_wrap_cpp(MOC_OUTFILES ${DESKTOPSTREAMER_MOC_HEADERS})
qt4_add_resources(QRC_SOURCES ${DESKTOPSTREAMER_RESOURCES})

if(APPLE)
  set(DESKTOPSTREAMER_APP_NAME DesktopStreamer)
  set(DESKTOPSTREAMER_ICON_FILE desktopstreamer.icns)

  list(APPEND DESKTOPSTREAMER_LINK_LIBRARIES
    "-framework Foundation -framework CoreGraphics")
  list(APPEND DESKTOPSTREAMER_SOURCES AppNapSuspender.mm)

  # set where in the bundle to put the icns file
  set_source_files_properties(${DESKTOPSTREAMER_ICON_FILE}
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  add_executable(${DESKTOPSTREAMER_APP_NAME} MACOSX_BUNDLE
    ${DESKTOPSTREAMER_SOURCES} ${MOC_OUTFILES} ${QRC_SOURCES}
    ${DESKTOPSTREAMER_ICON_FILE}) # include the icns file in the target

  # Configure the bundle property file using current version and year
  string(TIMESTAMP CURRENT_YEAR "%Y")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)
  set_target_properties(${DESKTOPSTREAMER_APP_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)

elseif(WIN32)
  set(DESKTOPSTREAMER_APP_NAME desktopstreamer)
  list(APPEND DESKTOPSTREAMER_LINK_LIBRARIES ${QT_QTMAIN_LIBRARY} Ws2_32)

  add_executable(${DESKTOPSTREAMER_APP_NAME} WIN32
    ${DESKTOPSTREAMER_SOURCES} ${MOC_OUTFILES} ${QRC_SOURCES})

else()
  set(DESKTOPSTREAMER_APP_NAME desktopstreamer)

  add_executable(${DESKTOPSTREAMER_APP_NAME}
    ${DESKTOPSTREAMER_SOURCES} ${MOC_OUTFILES} ${QRC_SOURCES})
endif()

target_link_libraries(${DESKTOPSTREAMER_APP_NAME} ${DESKTOPSTREAMER_LINK_LIBRARIES})

install(TARGETS ${DESKTOPSTREAMER_APP_NAME}
  RUNTIME DESTINATION bin COMPONENT desktopstreamer
  BUNDLE DESTINATION . COMPONENT desktopstreamer
)

include(CPackConfig)
