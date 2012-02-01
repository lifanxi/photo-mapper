/*
    PhotoMapper - Tag Photos with GPS data
    Copyright (C) 2007 Christoffer Hallqvist, COPIKS

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

#ifndef _PHOTO_H
#define _PHOTO_H

#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include <QPixmap>
#include <QDateTime>

#include "tag_dialog.h"

class Photo {
  public:

    struct pos {
      double lat;
      double lon;
      double alt;
      bool valid;
    };

	Photo ();
	~Photo();
    Photo(char *fileNameIn);
    void setGpsPos(double lat, double lon, double alt);
    void setGpsLat(double lat);
    void setGpsLon(double lon);
    void setGpsAlt(double alt);
    pos *getGpsPos();
    pos *getExifPos();
    bool hasGpsPos();
    bool hasExifPos();
    QPixmap getThumbnail();
	QSize getThumbnailSize();
	void setThumbnail(const QPixmap&);
    char *getFilename();
    QDateTime getExposureTime();
    int saveExifTags(tag_dialog::ModificationState mState=tag_dialog::SAVE_MODIFICATION_DATE);
    void setGpsPolyIndex(int idx);
    int getGpsPolyIndex();
    int getExifRotation();
  private:
    pos gpsPos;
    pos exifPos;
    QPixmap thumbnail;
    char *filename;
    QDateTime *expTime; // Exposure time (from EXIF)
	QDateTime *modTime; // File's modification time
	QDateTime *acTime; // File's access time
    Exiv2::Image::AutoPtr image;
	//Exiv2::ExifData exifData;
    int gpsPolyIndex;
    bool latvalid;
    bool lonvalid;
    bool altvalid;
	void readExifData();
    int exifOrientation;
};
 Q_DECLARE_METATYPE(Photo*)

#endif
