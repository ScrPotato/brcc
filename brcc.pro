TEMPLATE = app
TARGET = brcc

CONFIG += console
CONFIG += c++20
CONFIG -= qt

QMAKE_CXXFLAGS_RELEASE += -Ofast -Wall -Wa,-mbig-obj -flto
QMAKE_CXXFLAGS_DEBUG += -O0 -g -Wall -Wa,-mbig-obj -DDEBUG

QMAKE_LFLAGS += -static
QMAKE_LFLAGS_RELEASE += -O3 -flto
QMAKE_LFLAGS_DEBUG += -O0 -g

SOURCES += main.cpp
