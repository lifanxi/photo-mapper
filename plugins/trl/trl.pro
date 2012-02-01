TEMPLATE = lib
CONFIG += plugin
INCLUDEPATH += ../../PhotoMapper ../..
HEADERS = trl.h
SOURCES = trl.cpp
TARGET = trl
DESTDIR = ../
LIBS += -L../../libs -lgpsdata
