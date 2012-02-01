#include <gpx.h>
#include <QtXml>
#include <QDomDocument>


QStringList GpxPlugin::extensions() {
  QStringList returnList;
  returnList << "gpx";
  return returnList;
}

QString GpxPlugin::description() {
  return QString("GPS Exchange format");
}

bool GpxPlugin::identify(char *data, qint64 size) {
  char tmpStr[1024];
  for (int i=0; i<(size-3); i++) {
    if (data[i]=='<') {
      sscanf(data+i+1,"%s",tmpStr);
      if (strlen(tmpStr)==3) {
        if ((tmpStr[0]=='g' || tmpStr[0]=='G') && 
            (tmpStr[1]=='p' || tmpStr[1]=='P') && 
            (tmpStr[2]=='x' || tmpStr[2]=='X')) return true; 
      }
    }
  }
  return false;
}

QString GpxPlugin::errorMessage() {
  return _errorMessage;
}

QList<GpsPolygon*> GpxPlugin::readFile(QString fileName) {
  QList<GpsPolygon*> retList=QList<GpsPolygon*>();
  
  GpxHandler handler(&retList);
  QXmlSimpleReader reader;
  reader.setContentHandler(&handler);
  reader.setErrorHandler(&handler);

  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
	printf("Failed to open Gpx file\n");
	return retList;
  }

  QXmlInputSource xmlInputSource(&file);
  reader.parse(xmlInputSource);
	  
	return retList;
}

//==========================================================
GpxHandler::GpxHandler(QList<GpsPolygon *> *pList) {
  trktag = false;
  trkpttag = false;
  timetag = false;
  wpttag = false;
  tmpList = pList;
  wptList=QList<GpsPoint *>();
}

bool GpxHandler::startElement(const QString & /* namespaceURI */,
							  const QString & /* localName */,
							  const QString &qName,
							  const QXmlAttributes &attributes) {
  if (qName == "trk") {
	trktag = true;
  } 
  else if (qName == "trkpt") {
    trkpttag = true;
	lat = attributes.value("lat").toDouble();
	lon = attributes.value("lon").toDouble();
  }
  else if (qName == "ele") {
    eletag = true;
  }
  else if (qName == "time") {
  	time = QDateTime(); // Invalidate time
    timetag = true;
  } 
  else if (qName == "wpt") {
    wpttag = true;
    lat = attributes.value("lat").toDouble();
	lon = attributes.value("lon").toDouble();
  }
  return true;  
}

bool GpxHandler::endElement(const QString & /* namespaceURI */,
                              const QString & /* localName */,
                              const QString &qName)
 {
   if (qName == "trk") {
	 trktag = false;
	 if (!tmpPntList.isEmpty()) {
	   GpsPolygon *gpsPoly = new GpsPolygon(tmpPntList);
	   tmpList->append(gpsPoly);
	   tmpPntList=QList<GpsPoint *>();
	 }
   } 
   else if (qName == "trkpt") {
     trkpttag = false;
	 if (time.isValid()) {
	   GpsPoint *tmpPoint = new GpsPoint(time,lat,lon,alt);
	   tmpPntList.append(tmpPoint);
	 }
   }
   else if (qName == "wpt") {
     wpttag = false;
     if (time.isValid()) {
       GpsPoint *tmpPoint = new GpsPoint(time, lat, lon, alt);
       wptList.append(tmpPoint);
     }
   }
   else if (qName == "ele") {
     eletag = false;
   }
   else if (qName == "time") {
     timetag = false;
   } else if (qName == "gpx") {
     // End of gpx file, check if we have read any optional wpt tags
     if (!wptList.isEmpty()) {
       GpsPolygon *gpsPoly = new GpsPolygon(wptList);
	   tmpList->append(gpsPoly);
     }
   }
   return true;
 }

 bool GpxHandler::characters(const QString &str)
 {
   if ((trktag && trkpttag)||wpttag) {
	 if (eletag) {
	   alt = str.toDouble();
	 } else
	 if (timetag) {
	   int sign=1;
	   //char splitChar=0;
	   //QStringList strList = str.split(splitChar);
       QString timeStrPart1 = str.left(19);
       QString timeStrPart2 = str.right(str.length()-19);
  
       time = QDateTime::fromString(timeStrPart1,"yyyy-MM-dd'T'HH:mm:ss");
       time.setTimeSpec(Qt::UTC);

       if (timeStrPart2.at(0) == '+') sign=-1;
       if (timeStrPart2.at(0) == '-') sign=1;
  
       if (timeStrPart2.at(0) != 'Z') {
         QTime timeDiff = QTime::fromString(timeStrPart2.right(5),"hh:mm");
         QTime zeroTime(0,0);
  
         int timeDiffSecs = (-timeDiff.secsTo(zeroTime))*sign;
  
         time = time.addSecs(timeDiffSecs);
       }
	 }
   } 

   return true;
 }

 bool GpxHandler::fatalError(const QXmlParseException &exception)
 {
     return false;
 }


Q_EXPORT_PLUGIN2(Gpx, GpxPlugin)
