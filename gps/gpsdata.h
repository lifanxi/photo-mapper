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

#ifndef GPS_DATA_H
#define GPS_DATA_H

#include <QTime>
#include <QList>

class GpsPoint {
  public:
    GpsPoint();
    GpsPoint(QDateTime t, double lat, double lon, double alt);
    double getLat();
    double getLon();
    double getAlt();
    QDateTime getTime() const;

    bool operator<(const GpsPoint &compare);
  private:
    double lat;
    double lon;
    double alt;

    QDateTime time;
};

class GpsPolygon {
  public:
    GpsPolygon();
	GpsPolygon(QList<GpsPoint*> pList);
    bool getPositionFromTime(const QDateTime t, double *lat, double *lon, double *alt);
    double getMinLat();
    double getMinLon();
    double getMaxLat();
    double getMaxLon();
	double getMidLat();
	double getMidLon();
	QDateTime getMinTime() const;
	QDateTime getMaxTime() const;
    QList<GpsPoint *> getGpsPointList();
	
	bool operator<(const GpsPolygon &compare);
  private:
    QList<GpsPoint *> points;
    double latRange[2];
    double lonRange[2];
};


#endif
