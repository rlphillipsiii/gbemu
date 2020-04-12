STUB_CLOCKINTERFACE=true
STUB_MEMCONTROLLER=true

include(../test.pri)

SOURCES += opcodes.cpp
SOURCES += processor.cpp
SOURCES += timermodule.cpp

TARGET = cputest
