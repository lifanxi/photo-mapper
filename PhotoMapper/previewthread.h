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

#ifndef _PREVIEWTHREAD_H
#define _PREVIEWTHREAD_H

#include <QThread>
#include <photo.h>
#include <QWaitCondition>
#include <QMutex>

class PreviewThread : public QThread {
  Q_OBJECT
  
  public:
    PreviewThread(QObject *parent=0);
	~PreviewThread();
	void newSelection(Photo *photo, QSize);
  
  protected:
    void run();
	
  signals:
    void updateImage(const QImage &image, Photo *);
	
  private:
	QWaitCondition condition;
	QMutex mutex;
	Photo *currentPhoto;
	bool stopThread;
	QSize widgetSize;
};

#endif
