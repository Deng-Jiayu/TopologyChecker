
########################################################
# Files

SET (topologychecker_SRCS
     topologychecker.cpp
     topologycheckergui.cpp
)

SET (topologychecker_UIS topologycheckerguibase.ui)

SET (topologychecker_MOC_HDRS
     topologychecker.h
     topologycheckergui.h
)

SET (topologychecker_RCCS  topologychecker.qrc)

########################################################
# Build

QT4_WRAP_UI (topologychecker_UIS_H  ${topologychecker_UIS})

QT4_WRAP_CPP (topologychecker_MOC_SRCS  ${topologychecker_MOC_HDRS})

QT4_ADD_RESOURCES(topologychecker_RCC_SRCS ${topologychecker_RCCS})

ADD_LIBRARY (topologycheckerplugin MODULE ${topologychecker_SRCS} ${topologychecker_MOC_SRCS} ${topologychecker_RCC_SRCS} ${topologychecker_UIS_H})

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ../../core ../../core/raster
     ../../gui
     ..
)

TARGET_LINK_LIBRARIES(topologycheckerplugin
  qgis_core
  qgis_gui
)


########################################################
# Install

INSTALL(TARGETS topologycheckerplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

