#ifndef _MAPWIDGET_H
#define _MAPWIDGET_H

#include <QWebView>
#include <gps/gpsdata.h>
#include <photo.h>

class MapWidget: public QWebView {
  Q_OBJECT
  public:
    MapWidget(QWidget *parent=0);
    void setGpsPolygons(QList<GpsPolygon*>*);
    void deleteMapPolygon(GpsPolygon *gp);
    void addPhotoMarker(Photo *);
    void zoomToBox(double,double,double,double);

  public slots:
    void addPolygons(QList<int>);
    void removePolygons(QList<int>);
    void pageLoaded(bool);
    void deletePhotoMarker(Photo *);
    void selectPhoto(Photo *);
    void unselectPhoto(Photo *);

  protected:
    QWidget *iParent;
    void zoomToPolygons();

    QString generatePolyLine(GpsPolygon *p);
    void resizeEvent(QResizeEvent *event);
    QList<GpsPolygon*> *gpsPolys;
    QList<GpsPolygon*> selectedPolys;
    QList<GpsPoint *> reducePoints(QList<GpsPoint *> inPoly, 
                                   float reductionFactor, int maxVertices); 
    // Google maps polyline encoding methods
    QString encodePoints(GpsPolygon *gp);
    QString encodeLevels(GpsPolygon *gp);
    QString encodeNumber(unsigned int num);
    QString encodeSignedNumber(int num);

};

#endif
