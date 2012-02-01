#include <nmea.h>
#include <QFile>
#include <QTextStream>
#include <stdlib.h>

QStringList NmeaPlugin::extensions() {
  QStringList returnList;
  returnList << "nmea" << "nma" << "log";
  return returnList;
}

QString NmeaPlugin::description() {
  return QString("NMEA and Sony GPS log");
}


bool NmeaPlugin::identify(char *fileData, qint64 size) {
  if (strstr(fileData,"$GPRMC")) return true;
  return false;
}

QString NmeaPlugin::errorMessage() {
  return _errorMessage;
}


QList<GpsPolygon*> NmeaPlugin::readFile(QString fileName) {
  QList<GpsPolygon*> retList=QList<GpsPolygon*>();
  QDateTime dt=QDateTime();
  QDateTime oldDateTime = QDateTime();
  QDate d=QDate();
  QTime t=QTime();
  QList<GpsPoint*> pntList = QList<GpsPoint*>();
  
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
	printf("Failed to open NMEA file\n");
	return retList;
  }
  
  QTextStream ts(&file);
  QString line;
  do {
	line = ts.readLine();
	QStringList tagFields = line.split(',');
        if (tagFields.count()<10) continue; // Prevent crash on bad data
	if (tagFields.at(0) == "$GPRMC") {
	  if (tagFields.at(2) == "V") continue; // V indicates invalid data
	  double lat = (tagFields.at(3).left(2).toDouble() + 
				   tagFields.at(3).right(tagFields.at(3).length()-2).toDouble()/60.0);		
	  lat *= (tagFields.at(4)=="S"?-1:1);
	  QStringList lonParts = tagFields.at(5).split('.');
          if (lonParts.count() < 2) continue; // Prevent crash on bad data
	  QString lastTwo = lonParts.at(0).right(2);
	  QString firstPart = lonParts.at(0).left(lonParts.at(0).length()-2);
	  QString decimalPart = "." + lonParts.at(1);
	  double lon = firstPart.toDouble() +
				   (lastTwo.toDouble()+decimalPart.toDouble())/60;
		
	  lon *= (tagFields.at(6)=="W"?-1:1);
	  double alt = 0;

	  QStringList timeParts = tagFields.at(1).split(".");
	  t=QTime::fromString(timeParts.at(0),"hhmmss");
	  d=QDate::fromString(tagFields.at(9),"ddMMyy");
	  dt = QDateTime(d.addYears(100),t,Qt::UTC); // Two digit year gives 20th century in Qt - 
	                                             //we probably want 21st.

	  if (dt.isValid()) {
		bool newPoly=false;
		if (abs(dt.secsTo(oldDateTime)) > 3600 ) { // 1 hr threshold for new poly
		  newPoly = true;
		}
		if (oldDateTime.isValid() && newPoly && !pntList.isEmpty()) {
		  GpsPolygon *poly = new GpsPolygon(pntList);
		  retList.append(poly);
		}
	    if (newPoly) { 
		  pntList = QList<GpsPoint*>();
		}
	  }
	 
	  GpsPoint *pnt = new GpsPoint(dt,lat,lon,alt);
	  pntList.append(pnt);
	  
	  oldDateTime = dt;
	}
  } while (!line.isNull());
  
  if (!pntList.isEmpty()) {
	GpsPolygon *poly = new GpsPolygon(pntList);
	retList.append(poly);
  }
  return retList;
}

Q_EXPORT_PLUGIN2(nmea, NmeaPlugin)
