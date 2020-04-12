include(../build.pri)

TEMPLATE = app

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

LIBS += -lgtest_main
LIBS += -lgtest

unix: QMAKE_LFLAGS += -pthread

DEFINES += UNIT_TEST

INCLUDEPATH += ../inc

!isEmpty(STUB_CLOCKINTERFACE) {
    INCLUDEPATH = ../stubs/clock $$INCLUDEPATH

    HEADERS += ../stubs/clock/clock.h
}

!isEmpty(STUB_MEMCONTROLLER) {
    INCLUDEPATH = ../stubs/memorycontroller $$INCLUDEPATH

    HEADERS += ../stubs/memorycontroller/memorycontroller.h
    SOURCES += ../stubs/memorycontroller/memorycontroller.cpp
}

SOURCES += main.cpp
