#ifndef _NMEA_H
#define _NMEA_H

#include <fileFormatInterface.h>
#include <QTime>
#include <gps/gpsdata.h>

class NmeaPlugin : public QObject,
                  public FileFormatInterface {
  Q_OBJECT
  Q_INTERFACES(FileFormatInterface)
  
  public:
    QStringList extensions();
    QString description();
    QList<GpsPolygon*> readFile(QString fileName);
    bool identify(char *data, qint64 size);
    QString errorMessage();

  private:
    QString _errorMessage;
};

#endif
