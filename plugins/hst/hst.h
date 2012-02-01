#ifndef _HST_H
#define _HST_H

#include <fileFormatInterface.h>
#include <QXmlDefaultHandler>
#include <QTime>
#include <gps/gpsdata.h>

class HstPlugin : public QObject,
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

class HstHandler: public QXmlDefaultHandler {
  public:
     HstHandler(QList<GpsPolygon*> *pList);

     bool startElement(const QString &namespaceURI, const QString &localName,
                       const QString &qName, const QXmlAttributes &attributes);
     bool endElement(const QString &namespaceURI, const QString &localName,
                     const QString &qName);
     bool characters(const QString &str);
     bool fatalError(const QXmlParseException &exception);
    // QString errorString() const;

  private:
	 bool runtag;
	 bool trackpointtag;
	 bool longitudetag;
	 bool latitudetag;
	 bool altitudetag;
	 bool timetag;
	 
	 double lat;
	 double lon;
	 double alt;
	 QDateTime time;
	 QList<GpsPoint *> tmpPntList;
	 
	 QList<GpsPolygon *> *tmpList;
};

#endif
