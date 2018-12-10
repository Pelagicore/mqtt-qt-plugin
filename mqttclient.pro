# Copyright (C) Luxoft Sweden AB 2018

TEMPLATE = lib
TARGET = mqttclient
QT += qml quick network
CONFIG += plugin c++11

MOC_DIR = .moc
OBJECTS_DIR = .obj

TARGET = $$qtLibraryTarget($$TARGET)
uri = mqttclient

# Input
SOURCES += \
        mqttclient_plugin.cpp \
        mosquittoclient.cpp

HEADERS += \
        mqttclient_plugin.h \
        mosquittoclient.h

DISTFILES = qmldir

LIBS += -L"/usr/lib/x86_64-linux-gnu" -lmosquitto

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$OUT_PWD/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) \"$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)\"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

qmldir.files = qmldir
unix {
    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)
    qmldir.path = $$installPath
    target.path = $$installPath
    INSTALLS += target qmldir
}
