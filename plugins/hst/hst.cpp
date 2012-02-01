#include <hst.h>
#include <QtXml>
#include <QDomDocument>


QStringList HstPlugin::extensions() {
  QStringList returnList;
  returnList << "hst";
  return returnList;
}

QString HstPlugin::description() {
  return QString("Training Center History v1");
}

QString HstPlugin::errorMessage() {
  return _errorMessage;
}

bool HstPlugin::identify(char *data, qint64 size) {
  QString stringData(data);
  if (stringData.contains(QString("TrainingCenterDatabase/v1"),Qt::CaseInsensitive)) return true;
  return false;
}


QList<GpsPolygon*> HstPlugin::readFile(QString fileName) {
  _errorMessage = QString();
  QList<GpsPolygon*> retList=QList<GpsPolygon*>();
  
  HstHandler handler(&retList);
  QXmlSimpleReader reader;
  reader.setContentHandler(&handler);
  reader.setErrorHandler(&handler);

  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
        _errorMessage=QString("Failed to open ") + fileName + QString("\n");
	return retList;
  }

  QXmlInputSource xmlInputSource(&file);
  reader.parse(xmlInputSource);
	  
  return retList;
}

//==========================================================
HstHandler::HstHandler(QList<GpsPolygon *> *pList) {
  runtag = false;
  trackpointtag = false;
  latitudetag = false;
  longitudetag = false;
  altitudetag = false;
  timetag = false;
  tmpList = pList;
}

bool HstHandler::startElement(const QString & /* namespaceURI */,
							  const QString & /* localName */,
							  const QString &qName,
							  const QXmlAttributes &attributes) {
  if (qName == "Run") {
	runtag = true;
  }
  else if (qName == "LatitudeDegrees") {
	latitudetag = true;
  }
  else if (qName == "LongitudeDegrees") {
	longitudetag = true;
  } 
  else if (qName == "Trackpoint") {
    trackpointtag = true;
  }
  else if (qName == "AltitudeMeters") {
    altitudetag = true;
  } 
  else if (qName == "Time") {
    timetag = true;
  }
  return true;  
}

bool HstHandler::endElement(const QString & /* namespaceURI */,
							const QString & /* localName */,
							const QString &qName)
 {
   if (qName == "Run") {
	 runtag = false;
	 if (!tmpPntList.isEmpty()) {
	   GpsPolygon *gpsPoly = new GpsPolygon(tmpPntList);
	   tmpList->append(gpsPoly);
	   tmpPntList=QList<GpsPoint *>();
	 }
   }
   else if (qName == "LatitudeDegrees") {
     latitudetag = false;
   }
   else if (qName == "LongitudeDegrees") {
     longitudetag = false;
   } 
   else if (qName == "Trackpoint") {
     trackpointtag = false;
	 if (time.isValid()) {
	   GpsPoint *tmpPoint = new GpsPoint(time,lat,lon,alt);
	   tmpPntList.append(tmpPoint);
	 }
   }
   else if (qName == "AltitudeMeters") {
     altitudetag = false;
   } 
   else if (qName == "Time") {
     timetag = false;
   }
   return true;
 }

 bool HstHandler::characters(const QString &str)
 {
   if (runtag && trackpointtag) {
	 if (latitudetag) lat = str.toDouble();  
	 else if (longitudetag) lon = str.toDouble();
	 else if (altitudetag) alt = str.toDouble();
	 else if (timetag) {
	   time = QDateTime::fromString(str,"yyyy-MM-dd'T'HH:mm:ss'Z'");
	   time.setTimeSpec(Qt::UTC);
	 }
   } 

   return true;
 }

 bool HstHandler::fatalError(const QXmlParseException &exception)
 {
     return false;
 }


Q_EXPORT_PLUGIN2(hst, HstPlugin)
