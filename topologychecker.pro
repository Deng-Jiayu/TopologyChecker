TEMPLATE = lib
TARGET = TopologyChecker

PROJECT_PATH = $$PWD
SDK_PATH = $$PROJECT_PATH/../../

CONFIG(debug, debug|release){
    DESTDIR = $$SDK_PATH/bin/debug/plugins
}
else{
    DESTDIR = $$SDK_PATH/bin/release/plugins
}

TRANSLATION_NAME = $$TARGET
TRANSLATION_LANGS += zh_CN
TRANSLATION_DIR = $$SDK_PATH/i18n/
TRANSLATION_DESTDIR = $$DESTDIR/../i18n/
include( $$SDK_PATH/include/qgisconfig.pri )
include( $$SDK_PATH/include/translations.pri )

HEADERS += \
    areacheck.h \
    check.h \
    checkcontext.h \
    checkdock.h \
    checker.h \
    checkerror.h \
    checkerutils.h \
    checkitemdialog.h \
    checkresolutionmethod.h \
    checkset.h \
    collapsiblegroupbox.h \
    convexhullcheck.h \
    danglecheck.h \
    duplicatecheck.h \
    featurepool.h \
    gapcheck.h \
    holecheck.h \
    layerselectiondialog.h \
    lengthcheck.h \
    linecoveredbyboundarycheck.h \
    linecoveredbylinecheck.h \
    lineendonpointcheck.h \
    lineinpolygoncheck.h \
    lineintersectioncheck.h \
    linelayerintersectioncheck.h \
    linelayeroverlapcheck.h \
    lineoverlapcheck.h \
    linesegment.h \
    lineselfintersectioncheck.h \
    lineselfoverlapcheck.h \
    pointduplicatecheck.h \
    pointinpolygoncheck.h \
    pointonboundarycheck.h \
    pointonlinecheck.h \
    pointonlineendcheck.h \
    pointonlinenodecheck.h \
    polygoncoveredbypolygoncheck.h \
    polygoninpolygoncheck.h \
    polygonlayeroverlapcheck.h \
    polygonoverlapcheck.h \
    pseudoscheck.h \
    pushbutton.h \
    resulttab.h \
    segmentlengthcheck.h \
    setuptab.h \
    topologychecker.h \
    turnbackcheck.h \
    vectordataproviderfeaturepool.h \
    vectorlayerfeaturepool.h \
    widget.h

SOURCES += \
    areacheck.cpp \
    check.cpp \
    checkcontext.cpp \
    checkdock.cpp \
    checker.cpp \
    checkerror.cpp \
    checkerutils.cpp \
    checkitemdialog.cpp \
    checkresolutionmethod.cpp \
    checkset.cpp \
    collapsiblegroupbox.cpp \
    convexhullcheck.cpp \
    danglecheck.cpp \
    duplicatecheck.cpp \
    featurepool.cpp \
    gapcheck.cpp \
    holecheck.cpp \
    layerselectiondialog.cpp \
    lengthcheck.cpp \
    linecoveredbyboundarycheck.cpp \
    linecoveredbylinecheck.cpp \
    lineendonpointcheck.cpp \
    lineinpolygoncheck.cpp \
    lineintersectioncheck.cpp \
    linelayerintersectioncheck.cpp \
    linelayeroverlapcheck.cpp \
    lineoverlapcheck.cpp \
    linesegment.cpp \
    lineselfintersectioncheck.cpp \
    lineselfoverlapcheck.cpp \
    pointduplicatecheck.cpp \
    pointinpolygoncheck.cpp \
    pointonboundarycheck.cpp \
    pointonlinecheck.cpp \
    pointonlineendcheck.cpp \
    pointonlinenodecheck.cpp \
    polygoncoveredbypolygoncheck.cpp \
    polygoninpolygoncheck.cpp \
    polygonlayeroverlapcheck.cpp \
    polygonoverlapcheck.cpp \
    pseudoscheck.cpp \
    pushbutton.cpp \
    resulttab.cpp \
    segmentlengthcheck.cpp \
    setuptab.cpp \
    topologychecker.cpp \
    turnbackcheck.cpp \
    vectordataproviderfeaturepool.cpp \
    vectorlayerfeaturepool.cpp \
    widget.cpp
     
RESOURCES += \
    topologychecker.qrc

FORMS += \
    checkdock.ui \
    checkitemdialog.ui \
    layerselectiondialog.ui \
    resulttab.ui \
    setuptab.ui \
    topologycheckerguibase.ui