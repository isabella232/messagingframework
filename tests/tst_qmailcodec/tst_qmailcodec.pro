TEMPLATE = app
CONFIG += qtestlib unittest qtopiamail
TARGET = tst_qmailcodec

target.path += $$QMF_INSTALL_ROOT/tests

QTOPIAMAIL=../../src/libraries/qtopiamail

DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL/build
QMAKE_LFLAGS += -Wl,-rpath,$$QTOPIAMAIL

SOURCES += tst_qmailcodec.cpp

include(../../common.pri)
