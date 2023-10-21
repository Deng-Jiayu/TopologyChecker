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
    duplicatecheck.h \
    featurepool.h \
    layerselectiondialog.h \
    lineintersectioncheck.h \
    linelayerintersectioncheck.h \
    linelayeroverlapcheck.h \
    linesegment.h \
    lineselfintersectioncheck.h \
    pointduplicatecheck.h \
    pointinpolygoncheck.h \
    pointonboundarycheck.h \
    pointonlinecheck.h \
    pointonlineendcheck.h \
    pointonlinenodecheck.h \
    pushbutton.h \
    resulttab.h \
    setuptab.h \
    topologychecker.h \
    vectordataproviderfeaturepool.h \
    vectorlayerfeaturepool.h \
    widget.h

SOURCES += \
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
    duplicatecheck.cpp \
    featurepool.cpp \
    layerselectiondialog.cpp \
    lineintersectioncheck.cpp \
    linelayerintersectioncheck.cpp \
    linelayeroverlapcheck.cpp \
    linesegment.cpp \
    lineselfintersectioncheck.cpp \
    pointduplicatecheck.cpp \
    pointinpolygoncheck.cpp \
    pointonboundarycheck.cpp \
    pointonlinecheck.cpp \
    pointonlineendcheck.cpp \
    pointonlinenodecheck.cpp \
    pushbutton.cpp \
    resulttab.cpp \
    setuptab.cpp \
    topologychecker.cpp \
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