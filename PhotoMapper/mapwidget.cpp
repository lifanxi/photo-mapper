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

#include "mapwidget.h" 
#include <QWebFrame>
#include <math.h>
#include <QSemaphore>
#include <QMessageBox>

QSemaphore jsSemaphore(1);

MapWidget::MapWidget(QWidget *parent): 
 QWebView(parent)  {
    iParent = parent;
    connect(this,SIGNAL(loadFinished(bool)),this,SLOT(pageLoaded(bool)));
    selectedPolys.clear();
}

void MapWidget::resizeEvent(QResizeEvent* e) {
  QWebView::resizeEvent(e);
  jsSemaphore.acquire(1);
  page()->mainFrame()->evaluateJavaScript(QString("setMapSize(%1,%2);").arg(width()-20).arg(height()-20));
  jsSemaphore.release(1);
}

void MapWidget::setGpsPolygons(QList<GpsPolygon*>*polys) {
   gpsPolys = polys;
}

QString MapWidget::generatePolyLine(GpsPolygon *p) {
  static int count=0;
  count++;

  QString str = QString("var polyline%1 = new GPolyline.fromEncoded({ color: \"#70ff00\",weight:  4, points: \"%2\",levels: \"%3\", zoomFactor: 32, numLevels: 4});\n").arg(count).arg(encodePoints(p)).arg(encodeLevels(p));
  str += QString("addPolyLine(%1,polyline%2);\n").arg((int)(p)).arg(count);

  return str;
}

void MapWidget::pageLoaded(bool ok) {
  if (ok) {
    page()->mainFrame()->evaluateJavaScript(QString("setMapSize(%1,%2);").arg(width()-20).arg(height()-20));
  } else {
    QMessageBox::information(this, "Failed to load map...", "Failed to load map. (Requires internet connection)" );
  }
}

void MapWidget::deleteMapPolygon(GpsPolygon *gp) {
  QString cmd = QString("removePolyLine(%1);").arg((int)gp);
  jsSemaphore.acquire(1);
  page()->mainFrame()->evaluateJavaScript(cmd);   
  jsSemaphore.release(1);
}

QString MapWidget::encodePoints(GpsPolygon *gp) {
  QList<GpsPoint *>pts = gp->getGpsPointList();
  QString encodedResult;
  GpsPoint *pt;
  int lat, lon;
  int plat = 0, plon = 0; // Previous lat and lon values
  int dlat, dlon; // Delta lat and lon values
  foreach(pt, pts) {
    lat = (int)(pt->getLat()*1e5); lon = (int)(pt->getLon()*1e5);
    dlat = lat - plat;
    dlon = lon - plon;
    encodedResult += encodeSignedNumber(dlat) + encodeSignedNumber(dlon);
    plat = lat;
    plon = lon;
  }
  return encodedResult;
}

QString MapWidget::encodeLevels(GpsPolygon *gp) {
  QList<GpsPoint *>pts = gp->getGpsPointList();
  QString encodedResult;
  GpsPoint *pt;
  int lat, lon;
  int plat = 0, plon = 0; // Previous lat and lon values
  int dlat, dlon; // Delta lat and lon values
  foreach(pt, pts) {
    lat = (int)(pt->getLat()*1e5); lon = (int)(pt->getLon()*1e5);
    dlat = lat - plat;
    dlon = lon - plon;

    if (abs(dlat)<100 && abs(dlon)<100) {
        encodedResult += encodeNumber(0);
    } else if (abs(dlat)<1000 && abs(dlon)<1000) {
        encodedResult += encodeNumber(1);
    } else if (abs(dlat)<10000 && abs(dlon)<10000) {
        encodedResult += encodeNumber(2);
    } else {
        encodedResult += encodeNumber(3);
    }
    plat = lat;
    plon = lon;
  }

  // End values should always be visible
  encodedResult.replace(0,1,encodeNumber(3));
  if (encodedResult.size()>1)
    encodedResult.replace(encodedResult.size()-1,1,encodeNumber(3));

  return encodedResult;
}

QString MapWidget::encodeNumber(unsigned int num) {
  QString str;
  unsigned int nextValue;
  while (num >= 0x20) {
    nextValue = (0x20 | (num & 0x1f)) + 63;
    if (nextValue==92)
        str += "\\";
    str += QChar::fromAscii(nextValue);
    num >>= 5;
  }

  int finalValue = num+63;
  if (finalValue==92)
        str += "\\";
  str += QChar(finalValue);
  return str;
}

QString MapWidget::encodeSignedNumber(int num) {
  unsigned int signedNumber = num << 1;
  if (num < 0) {
    signedNumber = ~signedNumber;
  }
  return encodeNumber(signedNumber);
}

void MapWidget::zoomToPolygons() {
  GpsPolygon *p;
  if (selectedPolys.count() == 0) {
    return;
  }
  double minlat=90, minlon=180, maxlat=-90, maxlon=-180;
  foreach(p, selectedPolys) {
    if (p->getMaxLat() > maxlat) maxlat = p->getMaxLat();
    if (p->getMaxLon() > maxlon) maxlon = p->getMaxLon();
    if (p->getMinLat() < minlat) minlat = p->getMinLat();
    if (p->getMinLon() < minlon) minlon = p->getMinLon();
  }
  zoomToBox(minlat,minlon,maxlat,maxlon);
}

void MapWidget::zoomToBox(double minlat, double minlon, double maxlat, double maxlon) {
  jsSemaphore.acquire(1);
  page()->mainFrame()->evaluateJavaScript(QString("setVisibleBox(%1,%2,%3,%4);").arg(minlat).arg(minlon).arg(maxlat).arg(maxlon));
  jsSemaphore.release(1);
}

void MapWidget::addPolygons(QList<int> addedItems) {
  GpsPolygon *p;
  for (int i=0; i<addedItems.count(); i++) {
      p = gpsPolys->at(addedItems.at(i));
      jsSemaphore.acquire(1);
      if (p) page()->mainFrame()->evaluateJavaScript(generatePolyLine(p));
      jsSemaphore.release(1);
      if (selectedPolys.indexOf(p) == -1) selectedPolys.append(p);
   }
  zoomToPolygons();
}

void MapWidget::removePolygons(QList<int> removedItems) {
  GpsPolygon *p;
  for (int i=0; i<removedItems.count(); i++) {
      p = gpsPolys->at(removedItems.at(i));
      deleteMapPolygon(p);
      if (selectedPolys.indexOf(p) != -1) selectedPolys.removeAt(selectedPolys.indexOf(p));
   }
  zoomToPolygons();
}

void MapWidget::addPhotoMarker(Photo *addedPhoto) {
  if (!addedPhoto->hasGpsPos() && !addedPhoto->hasExifPos()) return;
  Photo::pos *photoPos;
  if (addedPhoto->hasGpsPos()) {
    photoPos = addedPhoto->getGpsPos();
  } else {
    photoPos = addedPhoto->getExifPos();
  }

  QString scriptToRun = QString("myAddMarker(%1,%2,%3);").arg((int)addedPhoto).arg(photoPos->lat).arg(photoPos->lon);
  jsSemaphore.acquire(1);
  page()->mainFrame()->evaluateJavaScript(scriptToRun);
  jsSemaphore.release(1);
}

void MapWidget::deletePhotoMarker(Photo *deletedPhoto) {
  QString scriptToRun = QString("removeMarker(%1);").arg((int)deletedPhoto);
  jsSemaphore.acquire(1);
  page()->mainFrame()->evaluateJavaScript(scriptToRun);
  jsSemaphore.release(1);
}

void MapWidget::selectPhoto(Photo *selectedPhoto) {
  QString scriptToRun = QString("highlightMarker(%1);").arg((int)selectedPhoto);
  jsSemaphore.acquire(1);
  page()->mainFrame()->evaluateJavaScript(scriptToRun);
  jsSemaphore.release(1);
}

void MapWidget::unselectPhoto(Photo *selectedPhoto) {
  QString scriptToRun = QString("unhighlightMarker(%1);").arg((int)selectedPhoto);
  jsSemaphore.acquire(1);
  page()->mainFrame()->evaluateJavaScript(scriptToRun);
  jsSemaphore.release(1);
}
