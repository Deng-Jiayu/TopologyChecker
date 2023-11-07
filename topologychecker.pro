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

include( UI.pri )
include( topologychecker.pri )
include( vector.pri )


HEADERS +=

SOURCES +=

RESOURCES += \
    topologychecker.qrc

FORMS +=

DISTFILES +=
