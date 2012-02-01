/*
    PhotoMapper - Tag Photos with GPS data
    Copyright (C) 2009 Christoffer Hallqvist, COPIKS

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    www.copiks.se
*/

#include "gpsreadingthread.h"

void GpsReadingThread::setFileFormatInterface(FileFormatInterface *ffi) {
  _ffi = ffi; 
}

void GpsReadingThread::setGpsPolyList(QList<GpsPolygon*> *polys) {
  _polys = polys;
}

void GpsReadingThread::setFileName(QString *fileName) {
  _file = fileName;
}

void GpsReadingThread::run() {
  if (_ffi == NULL || _polys == NULL || _file == NULL) return;

  *_polys += _ffi->readFile(*_file);
}
