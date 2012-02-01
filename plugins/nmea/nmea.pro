TEMPLATE = lib
CONFIG += plugin
INCLUDEPATH += ../../PhotoMapper ../..
HEADERS = nmea.h
SOURCES = nmea.cpp
TARGET = nmea
DESTDIR = ../
LIBS += -L../../libs -lgpsdata