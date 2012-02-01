COPIKS PhotoMapper v0.7
=========================

(C) 2009, Christoffer Hallqvist, COPIKS

-------------------------------------------------------------
Product web site: http://software.copiks.se/photomapper/
Alternative URL: http://photomapper.se/

Help site: http://software.copiks.se/photomapper/help/v07/index.php
-------------------------------------------------------------
View COPYING for product license
-------------------------------------------------------------
Third party libraries needed to build this package:

* QuaZip 0.2.1 - http://sourceforge.net/projects/quazip/
    To create zip files (or actually Google Earth kmz files)
* Exiv2 0.18 - http://www.exiv2.org/
    To read and write EXIF data in JPEG files
* Qt 4.5.1 - http://www.qtsoftware.com/
    GUI toolkit and more. PhotoMapper is built with mingw compiler.
-------------------------------------------------------------
How to build:
This application has only been built with MinGW compiler 
on Win32 platforms. No support whatsoever will be provided
as to how to build this application on either windows
or other platforms unless provided with this package or
on http://software.copiks.se/

1. First build (or download) libexiv2.a and libquazip.a.
   You might need cygwin or MinGW+MSYS to do this on windows.

2. Edit PhotoMapper/PhotoMapper.pro to reflect the whereabouts of
   these libraries (and other files). Use QAssistant
   to find out how to edit pro-files.

3. Run qmake to create makefile in the root directory of
   the PhotoMapper distribution.

4. Run make to build application and plugins.

-------------------------------------------------------------
Directories:
./ (root directory):
Includes the main PhotoMapper.pro file, which builds both the
main application and plugins.

./plugins:
Contains plugins.pro which builds all available plugins.

./plugins/tcx:
Contains plugin to read tcx-files.

./plugins/hst:
Contains plugin to read hst-files.

./plugins/gpx:
Contains plugin to read gpx-files.

./plugins/nmea:
Contains plugin to read nmea and log-files.

./PhotoMapper:
Contains the main application.

./gps
Library for manipulation of gps data.
