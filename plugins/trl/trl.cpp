#include <trl.h>
#include <QFile>
#include <QTextStream>

// First test Apple's clever macro that's really a runtime test so
// that our universal binaries work right.
#if defined __BIG_ENDIAN__
#define i_am_little_endian !__BIG_ENDIAN__
#else
#if defined WORDS_BIGENDIAN
# define i_am_little_endian 0
#else
# define i_am_little_endian 1
#endif
#endif

typedef struct gnav_trl_s {
  unsigned int time;
  float lat;
  float lon;
  unsigned int alt;
} gnav_trl_t;

static void
le_write32(void *addr, const unsigned value)
{
  unsigned char *p = (unsigned char *) addr;
  p[0] = value;
  p[1] = value >> 8;
  p[2] = value >> 16;
  p[3] = value >> 24;
}

static float
endian_read_float(const void* ptr, int read_le)
{
  float ret;
  char r[4];
  const void *p;
  int i;

  if (i_am_little_endian == read_le) {
    p = ptr;
  } else {
    for (i = 0; i < 4; i++) {
      r[i] = ((char*)ptr)[3-i];
    }
    p = r;
  }

  memcpy(&ret, p, 4);
  return ret;
}

static signed int
le_read32(const void *addr)
{
  const unsigned char *p = (const unsigned char *) addr;
  return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static float
le_read_float(const void *ptr)
{
  return endian_read_float(ptr, 1);
}

static double
read_altitude(void* ptr)
{
  unsigned char* i = (unsigned char*) ptr;
  char buf[sizeof(float)];
  le_write32(&buf, i[2] << 24 | i[1] << 16 | i[0] <<8 | i[3]);
  return le_read_float(&buf);
}

QStringList TrlPlugin::extensions() {
  QStringList returnList;
  returnList << "trl";
  return returnList;
}

QString TrlPlugin::description() {
  return QString("TRL GPS log");
}

bool TrlPlugin::identify(char *fileData, qint64 size) {
  return false;
}

QString TrlPlugin::errorMessage() {
  return _errorMessage;
}

QList<GpsPolygon*> TrlPlugin::readFile(QString fileName) {
  QList<GpsPolygon*> retList=QList<GpsPolygon*>();
  QDateTime dt=QDateTime();
  QDateTime oldDateTime = QDateTime();
  QDate d=QDate();
  QTime t=QTime();
  QList<GpsPoint*> pntList = QList<GpsPoint*>();
  
  QByteArray ba = fileName.toLatin1();
  const char * finName = ba.data();

  FILE * fin = fopen(finName, "r");

  while (!feof(fin))
  {
    gnav_trl_t rec;
    if (fread(&rec, sizeof(rec), 1, fin) != 1)
    {
      if (feof(fin)) 
        break;
      printf("Error reading file %s\n", finName);
      return retList;
    }

    signed int sidt = le_read32(&rec.time);
    float lat = le_read_float(&rec.lat);
    float lng = le_read_float(&rec.lon);
    double alt = read_altitude(&rec.alt);  
    dt.setTime_t(sidt);
    GpsPoint *pnt = new GpsPoint(dt, lat, lng, alt);
    pntList.append(pnt);
  }

  fclose(fin);
  if (!pntList.isEmpty()) {
    GpsPolygon *poly = new GpsPolygon(pntList);
    retList.append(poly);
  }
  return retList;
}

Q_EXPORT_PLUGIN2(trl, TrlPlugin)
