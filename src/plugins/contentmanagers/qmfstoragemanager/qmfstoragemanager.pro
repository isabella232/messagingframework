TEMPLATE = lib 
TARGET = qmfstoragemanager
PLUGIN_TYPE = contentmanagers
PLUGIN_CLASS_NAME = QmfStorageManagerPlugin
load(qt_plugin)
QT = core qmfclient

HEADERS += qmfstoragemanager.h

SOURCES += qmfstoragemanager.cpp
