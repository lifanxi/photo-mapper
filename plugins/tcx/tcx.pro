TEMPLATE = lib
CONFIG += plugin
INCLUDEPATH += ../../PhotoMapper ../..
HEADERS = tcx.h
SOURCES = tcx.cpp
TARGET = tcx
DESTDIR = ../
LIBS += -L../../libs -lgpsdata
QT += xml