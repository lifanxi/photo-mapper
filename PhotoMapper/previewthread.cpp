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

#include "previewthread.h"

PreviewThread::PreviewThread(QObject *parent):
  QThread(parent),
  currentPhoto(NULL),
  stopThread(false),
  widgetSize(50,50)  
{
  
}

PreviewThread::~PreviewThread() {
  mutex.lock();
  stopThread = true;
  condition.wakeOne();
  mutex.unlock();
  wait();
}

void PreviewThread::newSelection(Photo *photo, QSize sz) {
  mutex.lock();
  if (!isRunning()) {
    start(LowPriority);
  }
  condition.wakeOne();
  currentPhoto = photo;
  widgetSize = sz;
  mutex.unlock();
}

void PreviewThread::run() {
  while (!stopThread) {
	mutex.lock();
	Photo *localPhoto = currentPhoto;
	mutex.unlock();
	if (localPhoto!=NULL) {
      QImage image(localPhoto->getFilename());
      QTransform rot = QTransform();
      int exifRot = localPhoto->getExifRotation();
      if (exifRot==8) {
        rot.rotate(-90.0);
      } else if (exifRot==3) {
        rot.rotate(180.0);
      } else if (exifRot==6) {
        rot.rotate(90.0);
      }

      image = image.transformed(rot);
	  QImage scaledImage;
	  if (currentPhoto == localPhoto) {
		scaledImage = image.scaled(widgetSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
	  }
	  if (currentPhoto == localPhoto) {
	    emit updateImage(scaledImage, localPhoto);
	  }
	}
	
	mutex.lock();
	if (!stopThread && (localPhoto == currentPhoto))
	  condition.wait(&mutex);
	mutex.unlock();
  }
  
}
