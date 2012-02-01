#ifndef _GPX_H
#define _GPX_H

#include <fileFormatInterface.h>
#include <QXmlDefaultHandler>
#include <QTime>
#include <gps/gpsdata.h>

class GpxPlugin : public QObject,
                  public FileFormatInterface {
  Q_OBJECT
  Q_INTERFACES(FileFormatInterface)
  
  public:
    QStringList extensions();
    QString description();
    QList<GpsPolygon*> readFile(QString fileName);
    QString errorMessage();
    bool identify(char *data, qint64 size);

  private:
    QString _errorMessage;

};

class GpxHandler: public QXmlDefaultHandler {
  public:
     GpxHandler(QList<GpsPolygon*> *pList);

     bool startElement(const QString &namespaceURI, const QString &localName,
                       const QString &qName, const QXmlAttributes &attributes);
     bool endElement(const QString &namespaceURI, const QString &localName,
                     const QString &qName);
     bool characters(const QString &str);
     bool fatalError(const QXmlParseException &exception);
    // QString errorString() const;

  private:
	 bool trktag;
	 bool trkpttag;
	 bool timetag;
	 bool eletag;
	 bool wpttag;
	 
	 double lat;
	 double lon;
	 double alt;
	 QDateTime time;
	 QList<GpsPoint *> tmpPntList;
	 QList<GpsPoint *> wptList;
	 
	 QList<GpsPolygon *> *tmpList;

};

#endif
