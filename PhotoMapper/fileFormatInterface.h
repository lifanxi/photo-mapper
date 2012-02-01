#ifndef _FILEFORMATINTERFACE_H
#define _FILEFORMATINTERFACE_H

#include <QtPlugin>
#include <QStringList>
#include <QList>
//#include "gpsdata.h"
class QString;
class GpsPolygon;

class FileFormatInterface {
  public: 
    virtual ~FileFormatInterface() {}
    virtual QStringList extensions()=0;
    virtual QString description()=0;
    virtual QList<GpsPolygon*> readFile(QString fileName)=0; 
    // New in 1.1 interface
    virtual bool identify(char *data, qint64 size)=0;
    virtual QString errorMessage()=0;
    
};

Q_DECLARE_INTERFACE(FileFormatInterface, 
"se.copiks.PhotoMapper.FileFormatInterface/1.1")

#endif
