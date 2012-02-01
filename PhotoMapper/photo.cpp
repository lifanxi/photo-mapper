/*
    PhotoMapper - Tag Photos with GPS data
    Copyright (C) 2008 Christoffer Hallqvist, COPIKS

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

#include "photo.h"
#include "mainwindow.h"

#include <utime.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

Photo::Photo() {
  gpsPos.valid = false;
  exifPos.valid = true;
  filename = NULL; //strdup(fileNameIn);
  expTime = NULL;
  modTime = NULL;
  acTime = NULL;
  exifOrientation = 1;
  lonvalid = latvalid = altvalid = false;

  gpsPolyIndex = -1;
}

Photo::~Photo() {

}

Photo::Photo(char *fileNameIn) {
  gpsPos.valid = false;
  exifPos.valid = true;
  filename = strdup(fileNameIn);
  expTime = NULL;
  modTime = NULL;
  acTime = NULL;
  
  lonvalid = latvalid = altvalid = false;
  exifOrientation = 1;
  gpsPolyIndex = -1;
  
  struct stat st;
  if (stat(fileNameIn,&st)==0) {
	modTime = new QDateTime;
	modTime->setTime_t(st.st_mtime);
	acTime = new QDateTime;
	acTime->setTime_t(st.st_atime);
  }
  
  readExifData();
}

void Photo::readExifData() {
  Exiv2::DataBuf dataBuf;
  QString dateTimeStr;
  
  image = Exiv2::ImageFactory::open(filename);
  assert(image.get() != 0);

  image->readMetadata();
  Exiv2::ExifData &exifData = image->exifData();
 
  Exiv2::ExifThumbC exifThumb(exifData);
 
  if (!strcmp(exifThumb.extension(),".jpg")) {
    dataBuf = exifThumb.copy();
    thumbnail.loadFromData(dataBuf.pData_,dataBuf.size_,"JPEG");
  }
    
  if (exifData.empty()) {
    exifPos.valid = false;
    exifPos.lat = exifPos.lon = exifPos.alt = 0.0;
    dateTimeStr = "0000:00:00 00:00:00";
  } else {
    Exiv2::Exifdatum& tag = exifData["Exif.Photo.DateTimeOriginal"];
	dateTimeStr = QString::fromStdString(tag.toString());
	
	expTime= new QDateTime();
    *expTime = QDateTime::fromString(dateTimeStr,"yyyy:MM:dd HH:mm:ss");  
    Exiv2::Exifdatum& tag2 = exifData["Exif.GPSInfo.GPSLatitude"];
    QString str = QString::fromStdString(tag2.toString());
    
    if (str.isEmpty()) {
      exifPos.valid = false;
      exifPos.lat = 0.0;
    } else {
      QString str2 = MainWindow::convertString(str, EXIF_LOLA);
      exifPos.lat = MainWindow::convertDegToDecimal(str2);
    }
    Exiv2::Exifdatum& tag3 = exifData["Exif.GPSInfo.GPSLongitude"];
    str = QString::fromStdString(tag3.toString());
    if (str.isEmpty()) {
      exifPos.valid = false;
      exifPos.lon = 0;
    } else {
      QString str2 = MainWindow::convertString(str, EXIF_LOLA);
      exifPos.lon = MainWindow::convertDegToDecimal(str2);
    }
    Exiv2::Exifdatum& tag4 = exifData["Exif.GPSInfo.GPSAltitude"];
	str = QString::fromStdString(tag4.toString());
    if (str.isEmpty()) {
      exifPos.alt = 0;
    } else {
      QString str2 = MainWindow::convertString(str, EXIF_ALT);
      exifPos.alt = MainWindow::convertDegToDecimal(str2);
    }
    if (exifPos.valid) { // This should not be necessary but this helps
		                 // when loading images rotated in ACDSee which
		                 // causes the program to crash.
      Exiv2::Exifdatum& tag5 = exifData["Exif.GPSInfo.GPSLatitudeRef"];
      QString s = QString::fromStdString(tag5.toString());
	  if (s=="S") exifPos.lat = -exifPos.lat;
	  
      Exiv2::Exifdatum& tag6 = exifData["Exif.GPSInfo.GPSLongitudeRef"];
      s = QString::fromStdString(tag6.toString());
      if (s=="W") exifPos.lon = -exifPos.lon;
      
      Exiv2::Exifdatum& tag7 = exifData["Exif.GPSInfo.GPSAltitudeRef"];
      s = QString::fromStdString(tag7.toString());
      if (s=="1") exifPos.alt = -exifPos.alt; // Below sea level

	  setGpsPos(exifPos.lat,exifPos.lon,exifPos.alt);
	}

    Exiv2::Exifdatum& tag8 = exifData["Exif.Image.Orientation"];
    exifOrientation = tag8.toLong();
    if (exifOrientation < 1 || exifOrientation > 8) exifOrientation=1;
  } 
}

void Photo::setGpsPos(double lat, double lon, double alt) {
  gpsPos.lat = lat;
  gpsPos.lon = lon;
  gpsPos.alt = alt;
  gpsPos.valid = true;
  latvalid = lonvalid = altvalid = true;
}

void Photo::setGpsLat(double lat) {
  gpsPos.lat = lat;
  latvalid=true;
  if (lonvalid && altvalid) gpsPos.valid = true;
}

void Photo::setGpsLon(double lon) {
  gpsPos.lon = lon;
  lonvalid = true;
  if (latvalid && altvalid) gpsPos.valid = true;
}

void Photo::setGpsAlt(double alt) {
  gpsPos.alt = alt;
  altvalid = true;
  if (latvalid && lonvalid) gpsPos.valid = true;
}


Photo::pos *Photo::getGpsPos() {
  return &gpsPos;
}

Photo::pos *Photo::getExifPos() {
  return &exifPos;
}

int Photo::getGpsPolyIndex() {
  return gpsPolyIndex;
}

void Photo::setGpsPolyIndex(int idx) {
  gpsPolyIndex = idx;
}

bool Photo::hasGpsPos() {
  return gpsPos.valid;
}

bool Photo::hasExifPos() {
  return exifPos.valid;
}

QPixmap Photo::getThumbnail() {
  return thumbnail;
}

QSize Photo::getThumbnailSize() {
  return thumbnail.size();
}

void Photo::setThumbnail(const QPixmap &p) {
  // Only set thumbnail if none was found in the exif data
  if (!thumbnail) {
	// Canon exif thumbs are usually 160x120 so use this size.
	thumbnail = p.scaled(160,120,Qt::KeepAspectRatio,Qt::FastTransformation);
  }
}

char *Photo::getFilename() {
  return filename;
}

QDateTime Photo::getExposureTime() {  
  if (expTime!=NULL && expTime->isValid()) return *expTime;
  if (modTime!=NULL && modTime->isValid()) return *modTime;
  return QDateTime::fromString("0000:00:00 00:00:00","yyyy:MM:dd HH:mm:ss");  
}

int Photo::saveExifTags(tag_dialog::ModificationState mState) {
  //Why is it necessary to read meta data again?
  image->readMetadata();
  Exiv2::ExifData &exifData = image->exifData();
 
  exifData["Exif.GPSInfo.GPSVersionID"] = 0x02020000;

  exifData["Exif.Image.Software"] = "Copiks PhotoMapper (software.copiks.se)";

  exifData["Exif.GPSInfo.GPSLatitude"]  = MainWindow::convertDegToExif(
                                          MainWindow::convertDecimalToDeg(gpsPos.lat)).
                                          toStdString();
  exifData["Exif.GPSInfo.GPSLatitudeRef"] = gpsPos.lat>=0?"N":"S";
  exifData["Exif.GPSInfo.GPSLongitude"] = MainWindow::convertDegToExif(
                                          MainWindow::convertDecimalToDeg(gpsPos.lon)).
                                          toStdString();
  exifData["Exif.GPSInfo.GPSLongitudeRef"] = gpsPos.lon>=0?"E":"W";
  exifData["Exif.GPSInfo.GPSAltitude"]  = MainWindow::convertDegToExif(
                                          QString::number(fabs(gpsPos.alt))).
                                          toStdString();
  exifData["Exif.GPSInfo.GPSAltitudeRef"] = gpsPos.alt>=0?0:1;
  image->setExifData(exifData);
  try {
    image->writeMetadata();
  } catch (Exiv2::AnyError& e) {
    return errno;
  }

  if (mState == tag_dialog::CHANGE_MODIFICATION_DATE) return 0;
	  
  // Restore original modification time
  if (modTime && modTime->isValid()) {
    struct utimbuf utb;
	if (acTime && acTime->isValid()) {
      utb.actime = acTime->toTime_t();
	} else {
	  utb.actime = modTime->toTime_t();
	}
	if (mState == tag_dialog::SAVE_EXPOSURE_DATE) {
	  utb.modtime = expTime->toTime_t();
	}
	if (mState == tag_dialog::SAVE_MODIFICATION_DATE) {
	  utb.modtime = modTime->toTime_t();
	}
	utime(filename,&utb);
  }
  
  return 0; 
}

int Photo::getExifRotation() {
  return exifOrientation;
}
