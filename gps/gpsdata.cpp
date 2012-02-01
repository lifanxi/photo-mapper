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

#include "gpsdata.h"
#include <QStringList>

//================================================================
// This file contains code to read selected parts from Garmin
// hst-files
//================================================================

GpsPoint::GpsPoint() {
  lat=lon=alt=0.0;
  time = QDateTime();
}

GpsPoint::GpsPoint(QDateTime t, double latIn, double lonIn, double altIn) {
  time = t;
  lat = latIn;
  lon = lonIn;
  alt = altIn;
}

double GpsPoint::getLat() {
  return lat;
}

double GpsPoint::getLon() {
  return lon;
}

double GpsPoint::getAlt() {
  return alt;
}

QDateTime GpsPoint::getTime() const {
  return time;
}

bool GpsPoint::operator<(const GpsPoint &compare) {
  return (compare.getTime() < this->getTime());
}

/*==========================================

===========================================*/

GpsPolygon::GpsPolygon() {
}

GpsPolygon::GpsPolygon(QList<GpsPoint*> pList) {
  points = pList;
  GpsPoint *pnt;
  if (!points.isEmpty()) {
	pnt = points.at(0);
	latRange[0]=latRange[1]=pnt->getLat();
	lonRange[0]=lonRange[1]=pnt->getLon();
	foreach(pnt, points) {
	  double tmplat = pnt->getLat();
	  double tmplon = pnt->getLon();
	  
	  if (tmplat < latRange[0]) latRange[0]=tmplat;
	  else if (tmplat > latRange[1]) latRange[1]=tmplat;
	  
	  if (tmplon < lonRange[0]) lonRange[0]=tmplon;
	  else if (tmplon > lonRange[1]) lonRange[1]=tmplon;
	}
  }
}

double GpsPolygon::getMinLat() {
  return latRange[0];
}

double GpsPolygon::getMaxLat() {
  return latRange[1];
}

double GpsPolygon::getMinLon() {
  return lonRange[0];
}

double GpsPolygon::getMaxLon() {
  return lonRange[1];
}

double GpsPolygon::getMidLat() {
  return (latRange[0]+latRange[1])/2;
}

double GpsPolygon::getMidLon() {
  return (lonRange[0]+lonRange[1])/2;
}

QDateTime GpsPolygon::getMinTime() const {
  if (points.isEmpty()) return QDateTime();
  return points.first()->getTime();
}

QDateTime GpsPolygon::getMaxTime() const {
  if (points.isEmpty()) return QDateTime();
  return points.last()->getTime();
}

QList<GpsPoint *> GpsPolygon::getGpsPointList() {
  return points;
}

//================================================================
// Return a position based on the given time - of course this
// assumes that GPS-time and camera time is synchronized.
//================================================================
bool GpsPolygon::getPositionFromTime(const QDateTime t, double *lat,
                                     double *lon, double *alt) {
  GpsPoint *before=NULL, *after=NULL;
  if (points.isEmpty()) return false;
  if (t >= points.first()->getTime() && t<=points.last()->getTime()) {
     QList<GpsPoint*>::iterator iter;
     for (iter = points.begin(); iter!=points.end(); iter++) {
        if ((*iter)->getTime() <= t ) before = *iter;
        if ((*iter)->getTime() >= t ) {
          after = *iter;
          if (before != NULL) break;
        }
     }
  } else {
    return false;
  }

  double llat, llon, lalt;

  float timeinterval = after->getTime().toTime_t() - before->getTime().toTime_t();
  float timefromstart = t.toTime_t()-before->getTime().toTime_t();
  float timeoffsetfactor = 0;
  if (timeinterval) timeoffsetfactor=timefromstart/timeinterval;

  double latinterval = after->getLat()-before->getLat();
  double loninterval = after->getLon()-before->getLon();
  double altinterval = after->getAlt()-before->getAlt();

  llat = before->getLat()+latinterval*timeoffsetfactor;
  llon = before->getLon()+loninterval*timeoffsetfactor;
  lalt = before->getAlt()+altinterval*timeoffsetfactor;
  *lat = llat;
  *lon = llon;
  *alt = lalt;

  return true;
}

//==========================================================
// Less than operator is needed for qSort
// a polygon is considered less than another polygon if
// the start time is earlier.
//==========================================================

bool GpsPolygon::operator<(const GpsPolygon &compare) {
  return (compare.getMinTime() < this->getMinTime());
}

