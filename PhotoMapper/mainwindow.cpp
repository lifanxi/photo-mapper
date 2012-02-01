/*
    PhotoMapper - Tag Photos with GPS data
    Copyright (C) 2009 Christoffer Hallqvist, COPIKS

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

#include <mainwindow.h>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include "photo.h"
#include <math.h>
#include <quazip.h>
#include <quazipfile.h>
#include <QMessageBox>
#include <QDesktopServices> // Requires Qt 4.2 or later
#include <time.h>
#include <sys/types.h>
#include <QtAlgorithms>
#include <QUrl>
#include <QPluginLoader> 
#include <QTreeWidgetItem>
#include <QWebFrame>
#include "gpsreadingthread.h"
#include "sleep.h"

// These are hard coded values to reference the column number
// for each field in the image table. Ugly, but works.
int FILENAME=0,TIME=1,CORRTIME=2, LAT=3,LATREF=4,LON=5,LONREF=6,ALT=7;

//=============================================================
// Constructor for MainWindow based on designer ui-file and
// done with the multiple inheritance approach.
//=============================================================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUi(this); // Sets up GUI done in Qt Designer

  // Use (or create) settings file stored in application directory.
  // The location is important so that we can delete the ini-file
  // on uninstall.
  settings = new QSettings(QCoreApplication::applicationDirPath()+"/photomapper.ini",QSettings::IniFormat);

  // This is a stupid way to layout the splitters but
  // I just can't figure out how to do this in designer
  QList<int> sizelist;
  sizelist.push_back(200);
  sizelist.push_back(410);
  splitter->setSizes(sizelist);

  // Minimize the size of E/W and N/S columns - designer
  // can't do this (?)
  imageFileTable->resizeColumnToContents(LATREF);
  imageFileTable->resizeColumnToContents(LONREF);

  // callbackDisabled is a "semaphore"-like protection for
  // deactivating callbacks when table items are changed
  // by the application and not the user.
  callbackDisabled = false;

  // The preview thread will generate the preview pixmaps when
  // image selection changes.
  previewThread = new PreviewThread(this);
 
  // Queued connection means that the slot will be receieved
  // when this class' thread is executing.
  connect(previewThread,SIGNAL(updateImage(const QImage&, Photo*)),
          this, SLOT(updateImage(const QImage&,Photo*)),Qt::QueuedConnection);
 
  //Make it possible to remove GPS tracks from the list
  connect(gpsTreeWidget,SIGNAL(removeSelected()),
          this, SLOT(removeSelectedTracks()));

  connect(gpsTreeWidget,SIGNAL(selectionDeleted(QList<int>)),
          mapView, SLOT(removePolygons(QList<int>)));

  connect(gpsTreeWidget,SIGNAL(selectionAdded(QList<int>)),
          mapView, SLOT(addPolygons(QList<int>)));

  connect(this, SIGNAL(photoUnselected(Photo*)),
          mapView, SLOT(unselectPhoto(Photo*)));

    connect(this, SIGNAL(photoSelected(Photo*)),
          mapView, SLOT(selectPhoto(Photo*)));

    connect(imageFileTable, SIGNAL(removeSelected()), this, SLOT(deleteImagesClicked()));

    connect(imageFileTable, SIGNAL(zoomToSelected()), this, SLOT(zoomToSelectedPhotos()));

  aboutD = new AboutDialog(this);
  geDialog = new googleEarthExport_dialog(this,settings);
  tagDialog = new tag_dialog(this);
  
  // Check if Daylight savings time is active
  time_t now;
  struct tm bdt;

  time(&now);
  bdt = *localtime(&now);
  
  // Find out computer timezone
  QDateTime t1 = QDateTime::currentDateTime(); 
  QDateTime t2(t1);
  
  t1 = t1.toUTC(); 
  t1.setTimeSpec(Qt::LocalTime); // Treat both times as local time
                                 // for comparison

  selectedTimeZone = 0;
  computerTimeZone = 0;
  int diffSec = t1.secsTo(t2);
  if (bdt.tm_isdst==1) diffSec-=3600;
  float timezone = ((float)diffSec)/3600.0;
  int hours = (int)timezone; // Whole hours
  QString str="";
  if (hours > 0) str="+";
  if (hours < 0) str="-";
  str.append(QString("%1").arg(hours));
  int secsLeft = diffSec - (hours*3600);
  
  // Add quarters of hours for "strange" timezones
  if (secsLeft > 1700 && secsLeft <1900) str+=":30";
  if (secsLeft > 850 && secsLeft < 950) str+=":15";
  if (secsLeft > 2650 && secsLeft < 2750) str+=":45";
  
  // Find and select text in timezone combo box
  int idx = timezoneCombo->findText(str);
  if (idx!=-1) {
	timezoneCombo->setCurrentIndex(idx);
	computerTimeZone = timezone; 
  } else {
    computerTimeZone = 0; // Disable time adjustment and assume UTC is used in cam
	idx = timezoneCombo->findText("0"); // This SHOULD be present in the comboBox
	timezoneCombo->setCurrentIndex(idx);
  }
 
  gpsTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

  timeFineAdjust = 0;
  timeFineAdjustSign = -1;
  
  filePlugins = QList<FileFormatInterface*>();
  
  loadPlugins();

}

//===========================================================
// Help function for qSort (LessThan)
// Since the polygon list is a list of pointers we can't
// use operator<
//===========================================================
 bool polyCompare(const GpsPolygon *p1, const GpsPolygon *p2)
 {
     return (p1->getMinTime() < p2->getMinTime());
 }

//=====================================================
// This is an auto-slot for when "import GPS data" is
// selected in the File menu
//=====================================================
void MainWindow::on_actionImport_GPS_data_activated() {
  QString filter=QString();
  QString allExtensions = QString();
  bool fileProcessed = false;
  FileFormatInterface *identifiedFile = NULL;
  int initialPolyCount = polys.count();

  QProgressDialog *pd;
  pd = new QProgressDialog("Importing GPS data",0,0,0);
  
  static QString gpsFilePath = settings->value("PhotoMapper/gpsdir",
		                               QDir::homePath()).toString();
 

  // Create file format filers for file dialog 
  foreach(FileFormatInterface *fi,filePlugins) {
    filter+=fi->description() + " (";
	foreach (QString tmpStr, fi->extensions()) {
	  filter+="*."+tmpStr+" ";
	  allExtensions += "*."+tmpStr+" ";
	}
	filter+=");;";
  } 
  
  filter.insert(0,"All supported formats ("+allExtensions+");;");
  filter.append("All files (*.*);;");
  
  QStringList sl = QFileDialog::getOpenFileNames(
              this,
              "Choose a file",
              gpsFilePath,
              filter);
  
  if (sl.count()==0) return;
  QString s;
  GpsReadingThread *grt = new GpsReadingThread();
  if (grt==NULL) return;
 
  foreach (s,sl) { 
    fileProcessed = false;
    identifiedFile = NULL;

    // Save path in app settings and get the file extension
    QFileInfo fi(s);
    gpsFilePath = fi.absolutePath();
    settings->setValue("PhotoMapper/gpsdir",gpsFilePath);
  
    QString loadedExt = s.split(".").last();
  
    // Open file and read initial data for identification 
    QFile f(s);
    char fileData[10000]={0};
    int bytesRead=0;

    int countBefore = polys.count();

    if (!f.open(QIODevice::ReadOnly)) {
      QMessageBox::warning(this,"Open failed","Could not open file "+s);
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);
      continue;
    } else {
      bytesRead = f.read(fileData,10000);
      fileData[bytesRead-1]=0; // Make sure data is null terminated
                               // for safe string operations in plugins
      f.close();
    
      // Iterate over plugins and read file if it can be identified
      foreach (FileFormatInterface *fi, filePlugins) {
        if (fi->identify(fileData,bytesRead)) {
          identifiedFile = fi;

          pd->setLabelText("Importing " + s + " as " + fi->description());
          pd->show();

  	  QApplication::setOverrideCursor(Qt::WaitCursor);

          // Setup thread to read data so that progress meter can
          // be updated in the meantime.
          grt->setFileName(&s);
          grt->setGpsPolyList(&polys);
          grt->setFileFormatInterface(fi);
          grt->start();

	  do {
            // Keep event loop going while thread is running
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,
                                            100); 
            Sleep::msleep(10); // sleep for 10 milliseconds to leave room
                                // for other thread
          } while(grt->isRunning());

          fileProcessed = true;
	  QApplication::restoreOverrideCursor();
          pd->hide();
        } 
      } 
    }

			 
    if (!fileProcessed) {
      // Iterate over all known extensions and see if loaded file matches 
      foreach (FileFormatInterface *fi, filePlugins) {
        if (fi->extensions().contains(loadedExt,Qt::CaseInsensitive)) {
          pd->setLabelText("Importing " + s + " as " + fi->description());
          pd->show();

  	  QApplication::setOverrideCursor(Qt::WaitCursor);

          // Setup thread to read data so that progress meter can
          // be updated in the meantime.
          grt->setFileName(&s);
          grt->setGpsPolyList(&polys);
          grt->setFileFormatInterface(fi);
          grt->start();

	  do {
            // Keep event loop going while thread is running
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,
                                            100); 
            Sleep::msleep(10); // sleep for 10 milliseconds to leave room
                           // for other thread
          } while(grt->isRunning());

          fileProcessed = true;
	  QApplication::restoreOverrideCursor();
          pd->hide();
        } 
      }
    }

    if (fileProcessed) { 
      if (polys.count() == countBefore) {
        QMessageBox::warning(this,"Nothing imported","No supported time stamped data was found in " + s + "." );
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);
      } 
    } else {
      QMessageBox::warning(this, "Nothing imported", "File " + s + " could not be imported."); 
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);
    }
 
  }
  
  delete grt;

  if (polys.count() == initialPolyCount) return;

  QApplication::setOverrideCursor(Qt::WaitCursor);
  // Make sure polys are sorted chronologically
  qSort(polys.begin(),polys.end(),polyCompare);
  mapView->setGpsPolygons(&polys);
  QApplication::restoreOverrideCursor();

  updateGpsPositions();
 
  // Populate GPS tree widget 
  GpsPolygon *poly;
  QString direction;
  QString longLat, longLon;
  double dlola;
  
  gpsTreeWidget->clear(); // Empty the widget since we'll rebuild it.
  
  // Populate the tree widget with all GPS-tracks.
  foreach (poly,polys) {
    QString str = poly->getMinTime().toLocalTime().toString("yyMMdd hh:mm");
    
    //Create short longitude value for top-level item
    dlola=poly->getMidLon();
    if (dlola<0) direction = "W";
    else direction = "E";
    dlola = fabs(dlola); // Use absolute value to avoid stuff like "-19 -30 W" 
    str += " " + QString("%1").arg((int)dlola,2,10,QChar('0'))+QChar(176)+
    QString("%1").arg((int)((dlola-(int)dlola)*60.0),2,10,QChar('0'))+"'"+direction;
    //Create long longitude value for child
    longLon = convertDecimalToDeg(dlola) + " " + direction;

    //Create short latitude value for top-level item
    dlola=poly->getMidLat();
    if (dlola<0) direction = "S";
    else direction = "N";
    dlola = fabs(dlola);
    str += " " + QString("%1").arg((int)dlola,2,10,QChar('0'))+QChar(176)+
    QString("%1").arg((int)((dlola-(int)dlola)*60.0),2,10,QChar('0'))+"'"+direction;
    //Create long latitude value for child
    longLat = convertDecimalToDeg(dlola) + " " + direction;
    
    // Create and add top level item.
    QTreeWidgetItem *wi = new QTreeWidgetItem(gpsTreeWidget);
    wi->setText(0,str);

    str = poly->getMinTime().toLocalTime().toString("yyMMdd hh:mm") + " - " +
	  poly->getMaxTime().toLocalTime().toString("yyMMdd hh:mm");
	  
    //Three child items to show detailed time and position.
    QTreeWidgetItem *wi2 = new QTreeWidgetItem((QTreeWidget*)0,QStringList(str));
    QTreeWidgetItem *wi3 = new QTreeWidgetItem((QTreeWidget*)0,QStringList(longLat));
    QTreeWidgetItem *wi4 = new QTreeWidgetItem((QTreeWidget*)0,QStringList(longLon));
    //Disable all three child items so that only top-level items can be selected.
    wi2->setFlags(0);
    wi3->setFlags(0);
    wi4->setFlags(0);
    
    // Add children to top level item
    wi->addChild(wi2);
    wi->addChild(wi3);
    wi->addChild(wi4);
    
    wi->setSelected(true);
  }

}

//=============================================
// Auto-slot activated on File/Quit
//=============================================
void MainWindow::on_actionQuit_activated() {
  exit(0);
}

//============================================================
// Function to convert strings, should be separated into
// separate functions for each type-conversion like all
// other conversions in PhotoMapper
//============================================================
QString MainWindow::convertString(QString qstr, e_stringType stype) {
  double lola[3]={0,0,0};
  QStringList list;
  double tmp1=0,tmp2=0;
  QString retstr;

  switch (stype) {
    case EXIF_LOLA: // Convert EXIF lola format to lat lon degree format
      // Split input string on whitespace
      list = qstr.split(QRegExp("\\s+"));

      for (int i = 0; i < list.size(); ++i) {
        QString s = list.at(i);
        // Split substrings on "/" to get factors in integer division
        QStringList sublist=s.split("/");
        if ( sublist.size() == 2 ) {
          tmp1=sublist.at(0).toFloat();
          tmp2=sublist.at(1).toFloat();
        }
        if (tmp2<0.5) tmp2=1.0;
        // Get the real float value for this part of the EXIF data
        lola[i]=tmp1/tmp2;
      }
      // Construct a string for use in image table. DD MM SS.SS
      retstr = QString("%1 %2 %3").arg(lola[0],2,'f',0,'0').arg(lola[1],2,'f',0,'0').
             arg(lola[2],2,'f',2,'0');
      return retstr;
      break;
    case EXIF_ALT: // Convert EXIF Altitude data to a float-string
      list = qstr.split("/");
      if (list.size()==2) {
        tmp1=list.at(0).toFloat();
        tmp2=list.at(1).toFloat();
      }
      if (tmp2<0.5) tmp2=1;
      retstr = QString::number(tmp1/tmp2);
      return retstr;
      break;
    case EXIF_DATE: // Leave date as it is
      return qstr;
      break;
    default:  // Just return input as default
      return qstr;
      break;
  }
  return QString("DUMMY"); // Avoid compiler warnings by returning something
}

//=========================================================
// Auto-slot for when File/import images is activated
//=========================================================
void MainWindow::on_actionImport_images_activated() {
  Photo *tmpPhoto;
  static QString imageFilePath = settings->value("PhotoMapper/imagedir",
		                               QDir::homePath()).toString();
  
  /*
   * http://maps.google.com/?ie=UTF8&ll=59.400889,17.949257&spn=0.056273,0.126343&z=13
   */
  QStringList sl = QFileDialog::getOpenFileNames(
              this,
              "Choose a file",
              imageFilePath,
              "Image-files (*.jpg)");

  if (sl.isEmpty()) return;
  
  QFileInfo fi(sl[0]);
  imageFilePath = fi.absolutePath();
  settings->setValue("PhotoMapper/imagedir",imageFilePath);
  
  int pdValue = 0;			  
			  
  // Progress dialog is necessary since importing many images
  // on slow drives can take some time
  QProgressDialog *pd;
  pd = new QProgressDialog("Importing images...",0,0,photoList.size()+sl.size()*5);
  pd->show();

  // Create Photo-objects for each selected file.
  // Adding images can actually create duplicate entries in
  // the image table.
  for (int i = 0; i < sl.size(); ++i) {
    tmpPhoto = new Photo(sl.at(i).toLatin1().data());
    photoList.push_back(tmpPhoto);
	pdValue+=4;
	pd->setValue(pdValue);
	pd->raise();
	pd->setLabelText(sl.at(i));
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);
  }

  // Disable callbacks while application is tampering with
  // the image table
  callbackDisabled = true;

  //Clear old table
  imageFileTable->clearContents();
  imageFileTable->setRowCount(photoList.size());

  QList<Photo*>::iterator phIter;
  QTableWidgetItem *newItem = NULL;
  Photo::pos *exifPos;
  
  pd->setLabelText("Building image table...");

  int row=0;
  
  // Disable sorting or else the "row" argument in setItem
  // will move around and will mess up the table data.
  imageFileTable->setSortingEnabled(false);
  
  // This STL-looping style with iterator is actually not needed
  // with QT-list, but the code works so why change it?
  for (phIter = photoList.begin();
       phIter!=photoList.end();
       phIter++, row++) {

    // Is this a memory leak or does clearContents actually delete
    // table widget items? Doesn't matter much though unless a huge
    // amount of images are handled.

    QFileInfo fi((*phIter)->getFilename()); 

    newItem = new QTableWidgetItem(fi.fileName());
    newItem->setToolTip((*phIter)->getFilename());

	QVariant var;
	var.setValue(*phIter);
	newItem->setData(Qt::UserRole,var);
	
    newItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    imageFileTable->setItem(row,FILENAME,newItem);

    newItem = new QTableWidgetItem((*phIter)->getExposureTime().
                                   toString("yyyy:MM:dd HH:mm:ss"));
    imageFileTable->setItem(row,TIME,newItem);

    exifPos = (*phIter)->getExifPos();

    newItem = new QTableWidgetItem(convertDecimalToDeg(exifPos->lat));
    imageFileTable->setItem(row,LAT,newItem);

    newItem = new QTableWidgetItem(exifPos->lat<0?"S":"N");
    imageFileTable->setItem(row,LATREF,newItem);

    newItem = new QTableWidgetItem(convertDecimalToDeg(exifPos->lon));
    imageFileTable->setItem(row,LON,newItem);

    newItem = new QTableWidgetItem(exifPos->lon<0?"W":"E");
    imageFileTable->setItem(row,LONREF,newItem);

    newItem = new QTableWidgetItem(QString::number(exifPos->alt));
	
    imageFileTable->setItem(row,ALT,newItem);
	pd->setValue(++pdValue);
	pd->raise();
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);
  }

  // Reenable sorting
  imageFileTable->setSortingEnabled(true);
	   
  // Re-enable callbacks for table changes
  callbackDisabled = false;

  updateGpsPositions();
}

//===============================================================
// This method is called when GPS-data or images are imported
// to add GPS-positions to columns in image table. GPS data will
// override previously tagged EXIF-positions in the table. (The
// image file will of course be left untouched by this method).
//===============================================================
void MainWindow::updateGpsPositions() {
  QBrush redBrush(QColor(200,0,0)); // GPS match in red
  QBrush blackBrush(QColor(0,0,0)); // no match shown in black
 
  double minlat=90, minlon=180, maxlat=-90, maxlon=-180;
 
  // Disable callbacks while application is modifying image table
  callbackDisabled = true;
  float timeZoneAdjust = (selectedTimeZone-computerTimeZone);
  bool gpsMatch=false;
 
  for (int row=0; row<imageFileTable->rowCount(); row++) {	
	QVariant var = imageFileTable->item(row,0)->data(Qt::UserRole);
	Photo *varPhoto = var.value<Photo*>();
	
    QDateTime t=varPhoto->getExposureTime();
	t=t.addSecs((int)(timeZoneAdjust*3600)); // Compensate for time zone
	t=t.addSecs(timeFineAdjust*timeFineAdjustSign); // Apply user time 
	                                                // compensation

	if (!imageFileTable->item(row,CORRTIME)) {
	  imageFileTable->setItem(row,CORRTIME,new QTableWidgetItem);
	}
	imageFileTable->item(row,CORRTIME)->setText(t.toString("yyMMdd hh:mm:ss"));
	
    QList<GpsPolygon*>::iterator iter;
    int idx=0;
	gpsMatch = false;
    for (iter=polys.begin(); iter!=polys.end(); iter++) {
      double lat, lon, alt;
		
      if ((*iter)->getPositionFromTime(t,&lat,&lon,&alt)) {
        varPhoto->setGpsPolyIndex(idx); // Save which poly photo was taken in
                                         // for presentation purposes
        varPhoto->setGpsPos(lat,lon,alt); // Save photo GPS pos

        if (lat > maxlat) maxlat = lat;
        if (lon > maxlon) maxlon = lon;
        if (lat < minlat) minlat = lat;
        if (lon < minlon) minlon = lon;
        mapView->addPhotoMarker(varPhoto);

        imageFileTable->item(row,LAT)->setForeground(redBrush);
        imageFileTable->item(row,LON)->setForeground(redBrush);
        imageFileTable->item(row,ALT)->setForeground(redBrush);
		imageFileTable->item(row,LATREF)->setText((lat<0)?"S":"N");
		imageFileTable->item(row,LONREF)->setText((lon<0)?"W":"E");
        imageFileTable->item(row,LAT)->setText(convertDecimalToDeg(lat));
        imageFileTable->item(row,LON)->setText(convertDecimalToDeg(lon));
        imageFileTable->item(row,ALT)->setText(QString::number(alt));
		gpsMatch = true;
		break;
      } 
      idx++;
    }
	if (!gpsMatch) {
	  imageFileTable->item(row,LAT)->setForeground(blackBrush);
	  imageFileTable->item(row,LON)->setForeground(blackBrush);
	  imageFileTable->item(row,ALT)->setForeground(blackBrush);
	  // Strings can't just be set to zero here in case photo has
	  // been tagged already.
	  Photo::pos *exifPos;
	  exifPos = varPhoto->getExifPos();
	  
	  imageFileTable->item(row,LAT)->setText(convertDecimalToDeg(exifPos->lat));
      imageFileTable->item(row,LON)->setText(convertDecimalToDeg(exifPos->lon));
      imageFileTable->item(row,ALT)->setText(QString::number(exifPos->alt));
	  imageFileTable->item(row,LATREF)->setText((exifPos->lat<0.0)?"S":"N");
	  imageFileTable->item(row,LONREF)->setText((exifPos->lon<0.0)?"W":"E");
	}
  }
  imageFileTable->resizeColumnsToContents();

  //mapView->update();
	   
  // reenable callbacks
  callbackDisabled = false;
}

//================================================================
// Auto-slot for when contents of a cell in the image table has
// been changed. This enables the possibility to manually tag
// images with position.
//=================================================================
void MainWindow::on_imageFileTable_cellChanged(int row, int col) {
  // Check if callbacks has been disabled
  if (callbackDisabled) return;

  QString val = imageFileTable->item(row,col)->text();

  QVariant var = imageFileTable->item(row,0)->data(Qt::UserRole);
  Photo *photo = var.value<Photo*>();
  Photo::pos *gpsPos =  photo->getGpsPos();
  double latRef = (gpsPos->lat<0)?-1.0:1.0;
  double lonRef = (gpsPos->lon<0)?-1.0:1.0;

  if (col==LAT) photo->setGpsLat(convertDegToDecimal(val)*latRef);
  if (col==LATREF) {
    if ((val=="N" && latRef<0) || (val=="S" && latRef>=0))
      photo->setGpsLat(-gpsPos->lat);
  }
  if (col==LON) photo->setGpsLon(convertDegToDecimal(val)*lonRef);
  if (col==LONREF) {
    if ((val=="E" && lonRef<0) || (val=="W" && lonRef>=0))
      photo->setGpsLon(-gpsPos->lon);
  }
  if (col==ALT) photo->setGpsAlt(val.toDouble());
}

//=============================================================
// Auto-slot for when selections in the image table changes
//=============================================================
void MainWindow::on_imageFileTable_itemSelectionChanged() {
   QList<QTableWidgetItem *> itemList = imageFileTable->selectedItems();

   // If anything is selected buttons should be enabled
   if (itemList.count()) {
     googleEarthButton->setEnabled(true);
     tagButton->setEnabled(true);
   } else {
     googleEarthButton->setEnabled(false);
     tagButton->setEnabled(false);
   }

   // Find out which rows are selected
   QTableWidgetItem *ti;
   int row;
   QSet<int> currentSelectedRows;
   foreach (ti, itemList) {
     row = imageFileTable->row(ti);
     currentSelectedRows.insert(row);
   }

   // Emit signals for unselected rows
   QSet<int> unselectedRows = selectedRows-currentSelectedRows;
   foreach (row, unselectedRows) {
     QVariant var = imageFileTable->item(row,0)->data(Qt::UserRole);
     emit(photoUnselected(var.value<Photo*>()));
   }

   // Emit signals for new selections
   QSet<int> newlySelectedRows = currentSelectedRows - selectedRows;
   foreach (row, newlySelectedRows) {
     QVariant var = imageFileTable->item(row,0)->data(Qt::UserRole);
     emit(photoSelected(var.value<Photo*>()));
   }

   selectedRows = currentSelectedRows;

   // Draw preview of image. Preview will only be shown on the image that was selected
   // first in the current selection.
   if (itemList.count()) {
     bool resizeThumb = false;
     QTableWidgetItem *item=itemList.at(0);
     row = imageFileTable->row(item);
	 
     QVariant var = imageFileTable->item(row,0)->data(Qt::UserRole);
     Photo *varPhoto = var.value<Photo*>();
	 
	 if (varPhoto->getThumbnailSize() != pictureLabel->size()) {
	   previewThread->newSelection(varPhoto,pictureLabel->size());
	   resizeThumb = true;
	 }
	 
	 if (varPhoto->getGpsPolyIndex() > -1 &&
	 	varPhoto->getGpsPolyIndex() < polys.count()) {
	    QTreeWidgetItem *treeItem = gpsTreeWidget->invisibleRootItem()->child(varPhoto->getGpsPolyIndex());
	    
            gpsTreeWidget->applicationSelect(treeItem,true);
         }
	 
     QApplication::setOverrideCursor(Qt::WaitCursor);
     // Get thumbnail might take a little while for large images if
     // it hasn't been previously displayed.
	 previewImage = varPhoto->getThumbnail();
	 if (!previewImage.isNull() && previewImage.size().width()>0) {
           previewImage = previewImage.transformed(getTransformation(varPhoto));
	   if (resizeThumb) 
             pictureLabel->setPixmap(previewImage.scaled(pictureLabel->size(),Qt::KeepAspectRatio));
	   else
             pictureLabel->setPixmap(previewImage);
	 } else {
	   pictureLabel->setText("Loading preview image...");
	 }
     QApplication::restoreOverrideCursor();
   } else {
     // If nothing is selected show logo instead
     QPixmap p(":/application_logo.png");
     if (!p.isNull()) 
       pictureLabel->setPixmap(p); 
     else
       pictureLabel->setText("Photo preview"); // Text fallback
   }
}

//============================================================
// Convert decimal lat/lon value to degree-format string
//============================================================
QString MainWindow::convertDecimalToDeg(double valueIn) {
  int deg;
  int min;
  double sec;
  double value=fabs(valueIn);

  deg = (int)value;
  min = (int)((value-deg)*60);
  sec = (((value-deg)*60)-min)*60;

  QString retStr=QString("%1 %2 %3").arg(deg,2,10,QChar('0')).arg(min,2,10,QChar('0')).
                 arg(sec,7,'f',4,'0');

  return retStr;
}

//=================================================================
// Convert degree format string (as found in image table) to
// EXIF format string.
//=================================================================
QString MainWindow::convertDegToExif(QString degPosStr) {
  QString exifStr;
  QStringList list = degPosStr.split(QRegExp("\\s+"));
  if (list.size() == 1) { //Probably an altitude
    exifStr=QString::number((int)(list.at(0).toDouble()*100.0))+"/100";
  } else { // Probably lat or lon
    for (int i = 0; i < list.size(); ++i) {
      if (i==0 || i==1) exifStr+=list.at(i)+"/1 ";
      if (i==2) exifStr+=QString::number((int)(list.at(i).toDouble()*100.0))+"/100";
    }
  }
  return exifStr;
}

//=============================================================
// Convert degree-format string to decimal/numeric value
//============================================================
double MainWindow::convertDegToDecimal(QString degPosStr) {
  double lola[3]={0,0,0};
  int loopLength=3;

  QStringList list = degPosStr.split(QRegExp("\\s+"));

  if (list.size() < 3) loopLength = list.size();

  for (int i = 0; i < loopLength; ++i) {
    QString s = list.at(i);
    lola[i] = s.toDouble();
  }
  return lola[0]+lola[1]/60+lola[2]/3600;
}

//===========================================================
// Auto-slot for when "Tag images" button is clicked. This
// will save exif data into all selected images
//===========================================================
void MainWindow::on_tagButton_clicked() {
  if (tagDialog->exec()==QDialog::Rejected) {
    return;
  }

  QList<QTableWidgetItem *> itemList = imageFileTable->selectedItems();
  QList<int> rowsToExport = selectedRows.toList();

  // Progress dialog is necessary since tagging several large images takes
  // time.
  QStringList errors;
  QProgressDialog *pd;
  int ret;
  pd = new QProgressDialog("Tagging images...",0,0,rowsToExport.count());
  pd->show();
  for (int i=0; i<rowsToExport.count(); i++) {
    int row = rowsToExport.at(i);
	QVariant var = imageFileTable->item(row,0)->data(Qt::UserRole);
	Photo *p = var.value<Photo*>();
        if (ret = p->saveExifTags(tagDialog->getModificationState())) {
          errors << imageFileTable->item(row,0)->text() + " " + QString(strerror(ret)) + "\n";
        }
    pd->setValue(i);
	pd->raise();
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);
  }
  if (errors.count()) {
    QString errorString = QString("Failed to tag ")+QString::number(errors.count()) + " images.\n\n"; 
    for (int i=0; i<errors.count(); i++) {
      if (i==10) { // List maximum 10 files
        errorString += QString("... (%1 file(s) not listed)\n").arg(errors.count()-10);
        break;
      } 
      errorString += errors.at(i);
    }
    QMessageBox::warning(this, "Tag failed", errorString); 
  }
  pd->hide();
  delete(pd);
}

//===============================================================
// Auto-slot for when Google earth button is clicked. This
// will create a kmz file containing a custom icon (same as
// application 16-pixel icon) thumbnails in
// descriptions and coordinates for photograph. Google earth
// can be launched automatically if selected.
//
//===============================================================
void MainWindow::on_googleEarthButton_clicked() {
  double lat=0,lon=0,alt=0;

  QList<QTableWidgetItem *> itemList = imageFileTable->selectedItems();
  QList<int> rowsToExport;
  int row;
  QImage img;
  char tmp[256];
  QImage geIcon(":/ge_icon.png");
  QProgressDialog *pd;
  QList<int> exportPolys;
  
  // Open google earth export dialog.
  if (geDialog->exec()==QDialog::Rejected) {
    return;
  }

  bool is3d = geDialog->get3DexportEnabled();
  
  QString filename = geDialog->getExportFilename();
  settings->setValue("GoogleEarth/kmzfile",filename);

  if (filename.isNull() || filename.isEmpty()) {
     QMessageBox::critical(this, "Google Earth Export",
                   "You must select a file to export to.",
                   QMessageBox::Ok);
     return;
  }

  QUrl expurl=QUrl();
  settings->setValue("GoogleEarth/hyperenabled",geDialog->hyperTextEnabled());
  if (geDialog->hyperTextEnabled()) {
    expurl = QUrl(geDialog->getHyperlinkDir());
    if (!expurl.isValid())
      expurl = QUrl::fromLocalFile(geDialog->getHyperlinkDir());
    if (!expurl.isValid()) {
      QMessageBox::critical(this, "Google Earth Export",
                   "Invalid URL specified, either change the URL or "
                   "disable hyper linking.",
                   QMessageBox::Ok);
      return;
    }
  }
  settings->setValue("GoogleEarth/hyperlinkpath",expurl.toString());

  // Quazip third party package is used for creating kmz
  // (which is actually a zip-file)
  // QuaZip is GNU GPL and can be found at
  // http://sourceforge.net/projects/quazip/
  QuaZip zip(filename);
  zip.open(QuaZip::mdCreate);
  QuaZipFile outFile(&zip);
  outFile.open(QIODevice::WriteOnly, QuaZipNewInfo("temp.kml"));

  // Check all rows that has at least one item selected
  for (int i=0; i<itemList.count(); i++) {
    row = imageFileTable->row(itemList.at(i));
    if (!rowsToExport.contains(row))
      rowsToExport.push_back(row);
  }

  // Sort the selected rows in case they were not selected in order.
  qSort(rowsToExport);

  // Progress dialog is necessary since creating thumbnails for many large
  // files might take a while.
  pd = new QProgressDialog("Exporting for Google Earth...",0,0,rowsToExport.count()+2);
  pd->show();

  // Start outputing the kml-file.
  outFile.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<kml xmlns=\"http://earth.google.com/kml/2.1\">\n"
           "<Document>");
  outFile.write("<Style id=\"photoMapperStyle\">\n");
  outFile.write("  <IconStyle>\n");
  outFile.write("     <Icon><href>ge_icon.png</href></Icon>\n");
  outFile.write("  </IconStyle>\n");
  outFile.write("</Style>\n");

  settings->setValue("GoogleEarth/iconsenabled",geDialog->getIconsEnabled());
  if (geDialog->getIconsEnabled()) {
      for (int i=0; i<rowsToExport.count(); i++) {
        row = rowsToExport.at(i);
        QStringList strList= imageFileTable->item(row,FILENAME)->text().split('/');
        sprintf(tmp,"<Style id=\"%s_ICON\">\n",strList.last().toLatin1().data());
        outFile.write(tmp);
        outFile.write("  <IconStyle>\n");
        sprintf(tmp,"     <Icon><href>ICON_%s</href></Icon>\n",strList.last().toLatin1().data());
        outFile.write(tmp);
        outFile.write("  </IconStyle>\n");
        outFile.write("</Style>\n");
      }
  }

  for (int i=0; i<rowsToExport.count(); i++) {
    row = rowsToExport.at(i);
	
	if (geDialog->getPolysEnabled()) {
	  // Find attached gpsPolygon for export
	  QVariant var = imageFileTable->item(row,0)->data(Qt::UserRole);
      Photo *p = var.value<Photo*>();
	  int tmpIdx=-1;
	  if ((tmpIdx=p->getGpsPolyIndex())!=-1) {
	    // only one entry per polygon should be exported	
	    if (!exportPolys.contains(tmpIdx)) 
  		  exportPolys.push_back(tmpIdx);
      }
	}
	
    QStringList strList= imageFileTable->item(row,FILENAME)->text().split('/');
	
	pd->setLabelText(strList.last());
	
    // Each photo is represented as a Placemark
    outFile.write("<Placemark>\n");

    if (geDialog->getIconsEnabled()) {
      sprintf(tmp,"<styleUrl>#%s_ICON</styleUrl>\n",strList.last().toLatin1().data());
      outFile.write(tmp);
    } else {
      // Use globaly defined icon
      outFile.write("<styleUrl>#photoMapperStyle</styleUrl>");
    }

    if (imageFileTable->item(row,FILENAME)) {
      outFile.write("<name>");
      outFile.write(strList.last().toLatin1().data()); // Filename w/o path
      outFile.write("</name>\n");
    }

    outFile.write("<description>\n");
    // Create reference to thumbnail relative to zip/kmz file
    // using html tags.
    sprintf(tmp,"<![CDATA[");
    outFile.write(tmp);
    if (geDialog->hyperTextEnabled()) {

      sprintf(tmp,"<a href=\"%s/%s\">",
              expurl.toString().toLatin1().data(),
              strList.last().toLatin1().data());
      outFile.write(tmp);
    }
    if (geDialog->thumbsEnabled()) {
      sprintf(tmp,"<img src=\"%s\">",
                  strList.last().toLatin1().data());
    } else {
      sprintf(tmp,"%s",strList.last().toLatin1().data());
    }
    outFile.write(tmp);

    if (geDialog->hyperTextEnabled()) {
      outFile.write("</a>");
    }
    outFile.write("<br><a href=\"http://software.copiks.se/photomapper/\">");
	outFile.write("COPIKS PhotoMapper</a>\n");
    sprintf(tmp,"]]>\n");

    outFile.write(tmp);
    outFile.write("</description>");
    // Google earth represents west as negative LONs and south as
    // negative LATs.
    if (imageFileTable->item(row,LAT)) {
      lat = convertDegToDecimal(imageFileTable->item(row,LAT)->text());
      if (imageFileTable->item(row,LATREF)->text()=="S") lat=-lat;
    }
    if (imageFileTable->item(row,LON)) {
      lon = convertDegToDecimal(imageFileTable->item(row,LON)->text());
      if (imageFileTable->item(row,LONREF)->text()=="W") lon=-lon;
    }
    if (imageFileTable->item(row,ALT)) {
      alt = imageFileTable->item(row,ALT)->text().toFloat();
    }
	outFile.write("<Point>\n");
	if (is3d) outFile.write("<altitudeMode>absolute</altitudeMode>\n");
    sprintf(tmp,"<coordinates>%f,%f,%f</coordinates>\n",
            lon,lat,alt);
    outFile.write(tmp);
    outFile.write("</Point></Placemark>\n");
    
  }
  
  pd->setLabelText("Exporting polygons...");
  // Export GPS polygons (if enabled in dialog)
  settings->setValue("GoogleEarth/polyenabled",geDialog->getPolysEnabled());
  settings->setValue("GoogleEarth/poly2d",!is3d);
  
  if (geDialog->getPolysEnabled()) {
	GpsPolygon *tmpPoly;
	QList<GpsPoint *> pointList;
	GpsPoint *tmpPoint;
	QRgb col= geDialog->getColor();
	outFile.write("<Style id=\"polyStyle\">\n");
    outFile.write("  <LineStyle>\n");
	sprintf(tmp,"    <color>%02x%02x%02x%02x</color>\n",
		    qAlpha(col),qBlue(col),qGreen(col),qRed(col));
	outFile.write(tmp);
	outFile.write("  <width>3</width>\n");
	outFile.write("  </LineStyle>\n");
	outFile.write("</Style>\n");
    for (int i=0; i<exportPolys.count(); i++) {
	  outFile.write("<Placemark>\n");
          outFile.write("  <styleUrl>#polyStyle</styleUrl>\n");
	  sprintf(tmp,"<Name>Poly %d</Name>\n",exportPolys.at(i));
	  outFile.write("<LineString>\n");
	  outFile.write("  <extrude>0</extrude>\n");
      if (is3d) {
	    outFile.write("  <tessellate>0</tessellate>\n");
	    outFile.write("  <altitudeMode>absolute</altitudeMode>\n");
	  } else {
		outFile.write("  <tessellate>1</tessellate>\n");
	  }
	  outFile.write("  <coordinates>\n");
	  tmpPoly = polys.at(exportPolys.at(i));
	  if (tmpPoly) {
		pointList = tmpPoly->getGpsPointList();
	    for (int j=0; j<pointList.count(); j++) {
		  tmpPoint = pointList.at(j);
		  sprintf(tmp,"    %f,%f,%f\n",tmpPoint->getLon(),tmpPoint->getLat(),tmpPoint->getAlt());   
		  outFile.write(tmp);
		}
	  }
	  outFile.write("  </coordinates>\n");
	  outFile.write("</LineString>\n");
	  outFile.write("</Placemark>\n");
	}
  } // End of polygon export.
  
  pd->setValue(2);
  outFile.write("</Document>\n</kml>\n");
  outFile.close();

  // Save the icon to the kmz-file
  outFile.open(QIODevice::WriteOnly, QuaZipNewInfo("ge_icon.png"));
  geIcon.save(&outFile,"PNG");
  outFile.close();

  
  // Generate thumbnails for kmz file
  settings->setValue("GoogleEarth/thumbsenabled",geDialog->thumbsEnabled());
  if (geDialog->thumbsEnabled()) {
  	int tnWidth = geDialog->getThumbnailWidth();
  	int tnHeight = geDialog->getThumbnailHeight();
  	settings->setValue("GoogleEarth/thumbheight",tnHeight);
  	settings->setValue("GoogleEarth/thumbwidth",tnWidth);  	
  	int jpegQuality = geDialog->getJpegQuality();
  	settings->setValue("GoogleEarth/jpegquality",jpegQuality);
    for (int i=0; i<rowsToExport.count(); i++) {
      row = rowsToExport.at(i);
      QVariant var = imageFileTable->item(row,0)->data(Qt::UserRole);
      Photo *varPhoto = var.value<Photo*>();

      QStringList strList= QString(varPhoto->getFilename()).split('/');
      pd->setLabelText(QString("Thumbnail: ") + strList.last());
      img = QImage(varPhoto->getFilename()).transformed(getTransformation(varPhoto)).scaled(
                         tnWidth,tnHeight,Qt::KeepAspectRatio);
      outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(strList.last()));
      img.save(&outFile,"JPG",jpegQuality);
      outFile.close();

      if (geDialog->getIconsEnabled()) {
        outFile.open(QIODevice::WriteOnly, QuaZipNewInfo("ICON_"+strList.last()));
        img.scaled(32,32,Qt::KeepAspectRatio).save(&outFile,"JPG",60);
        outFile.close();
      }

      pd->setValue(i+2);
      pd->raise();
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);
    }
  }

  zip.close(); // Done with zipping

  // Open the new file with Google Earth if available
  settings->setValue("GoogleEarth/launch",geDialog->getLaunchGoogleEarth());
  if ( geDialog->getLaunchGoogleEarth()) {
    QUrl url = QUrl::fromLocalFile(filename);
    if (!url.isValid()) printf("Invalid URL\n");
    if (!QDesktopServices::openUrl(url)) {
      QMessageBox::information(this,"Google Earth Export",
                               "Failed to launch Google Earth");
    }
  }

  // Get rid of progress dialog
  pd->hide();
  delete(pd);
}

//==========================================================
// Auto-slot for Help/Online Help. Launches a website
// connected to this application and version. Return value
// from openUrl is not reliable. On a computer without
// internet connection nothing will probably happen.
//===========================================================
void MainWindow::on_actionOnline_help_activated() {
  QUrl url = QUrl("http://copiks.com/software/photomapper/help/v07/");
  if (!QDesktopServices::openUrl(url)) {
     QMessageBox::information(this,"Help","Help is only available via www "
                              "and connection failed.");
  }
}

//=============================================================
// Auto slot for Help/contribute. Launches paypal contribution
// website copiks.
//=============================================================
void MainWindow::on_actionContribute_activated() {
  QUrl url = QUrl("http://software.copiks.com/photomapper/contribute.php");
  if (!QDesktopServices::openUrl(url)) {
     QMessageBox::information(this,"Contribute","Contributions can only "
                              "be done online and connection failed.");
  }
};

//==============================================================
// Auto-slot for Help/About
//==============================================================
void MainWindow::on_actionAbout_Copiks_PhotoMapper_activated() {
  if (aboutD!=NULL)
    aboutD->show();
}

//==============================================================
// Auto-slot for when remove images button is clicked. This
// will remove images from image table but will of course not
// touch the files
//==============================================================
void MainWindow::deleteImagesClicked() {
  QList<QTableWidgetItem *> itemList = imageFileTable->selectedItems();
  QList<int> rowsToDelete;

  for (int i=0; i<itemList.count(); i++) {
    int row = imageFileTable->row(itemList.at(i));
    if (!rowsToDelete.contains(row))
      rowsToDelete.push_back(row);
  }
  qSort(rowsToDelete);
  for (int i=rowsToDelete.count()-1; i>=0; i--) {
    QVariant var = imageFileTable->item(rowsToDelete.at(i),0)->data(Qt::UserRole);
    Photo *p = var.value<Photo*>();
    imageFileTable->removeRow(rowsToDelete.at(i));
    mapView->deletePhotoMarker(p);
    photoList.removeOne(p);
  }
}

//============================================
// Auto-slot for time zone selection comboBox
//============================================
void MainWindow::on_timezoneCombo_currentIndexChanged(QString s) {
  QStringList strList = s.split(QChar(':'));
  selectedTimeZone = 0;
  if (strList.count()>0) {
    selectedTimeZone = strList[0].toFloat();
  }
  if (strList.count()>1) {
    float minutes = strList[1].toFloat();
	float sign = selectedTimeZone<0?-1:1; // Make sure we substract from negative
	                                      // values and add to positive values.
	selectedTimeZone += (minutes/60.0)*sign;
  }
  updateGpsPositions();
  on_imageFileTable_itemSelectionChanged(); // manually update gps poly view
}

//Auto slot for fine time adjustment time edit
void MainWindow::on_timeCorrectionTimeEdit_timeChanged(const QTime t){
  timeFineAdjust = t.second() + 60*t.minute() + 3600*t.hour();
  updateGpsPositions();
  on_imageFileTable_itemSelectionChanged(); // manually update gps poly view
};

//Auto slot for time correction sign (sets if clock in camera is fast or slow)
void MainWindow::on_timeCorrectionSignCombo_activated(QString s) {
  if (s=="Fast") {
	timeFineAdjustSign = -1;
  }
  if (s=="Slow") {
	timeFineAdjustSign = 1;
  }
  on_imageFileTable_itemSelectionChanged(); // manually update gps poly view
  updateGpsPositions();
};

QTransform MainWindow::getTransformation(Photo *pht) {
  QTransform rot = QTransform();
  int exifRot = pht->getExifRotation();
  if (exifRot==8) {
    rot.rotate(-90.0);
  } else if (exifRot==3) {
    rot.rotate(180.0);
  } else if (exifRot==6) {
    rot.rotate(90.0);
  }
  return rot;
}

void MainWindow::updateImage(const QImage &img, Photo *pht) {
  QPixmap pm = QPixmap::fromImage(img);
  pictureLabel->setPixmap(pm);
  pht->setThumbnail(pm);
}

void MainWindow::loadPlugins() {
  QStringList filters;
  filters  << "*.dll";
  
  // Search through the library path for dll:s that are plugins
  foreach (QString path, qApp->libraryPaths()) {
	QDir pluginsDir = QDir(path);
    foreach (QString fileName, pluginsDir.entryList(filters,QDir::Files)) {
	  QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
	  QObject *plugin = loader.instance();
	  if (plugin) {
	    FileFormatInterface *fLoader = qobject_cast<FileFormatInterface *>(plugin);
        if (fLoader) {
	      filePlugins.append(fLoader);
	    }
      }
	}
  }
}

//========================================================
// Calcualte bounding box for selected Photographs and
// zoom map to this box
//========================================================
void MainWindow::zoomToSelectedPhotos() {
  QList<QTableWidgetItem *> itemList = imageFileTable->selectedItems();
  QList<int> rows;

  for (int i=0; i<itemList.count(); i++) {
    int row = imageFileTable->row(itemList.at(i));
    if (!rows.contains(row))
      rows.push_back(row);
  }

  double minlat=90, maxlat=-90, minlon=180, maxlon=-180;
  bool foundPos = false;
  for (int i=0; i<rows.count(); i++) {
    QVariant var = imageFileTable->item(rows.at(i),0)->data(Qt::UserRole);
    Photo *p = var.value<Photo*>();

    Photo::pos *pos;
    if (p->hasGpsPos()) {
      pos = p->getGpsPos();
      foundPos = true;
    } else if (p->hasExifPos()) {
      pos = p->getExifPos();
      foundPos = true;
    } else {
      continue;
    }

    if (pos->lat>maxlat) maxlat=pos->lat;
    if (pos->lon>maxlon) maxlon=pos->lon;
    if (pos->lat<minlat) minlat=pos->lat;
    if (pos->lon<minlon) minlon=pos->lon;
  }

  if (foundPos) {
    mapView->zoomToBox(minlat,minlon,maxlat,maxlon);
  }

}

//=====================================================
// Remove selected GPS tracks from tree widget
//=====================================================
void MainWindow::removeSelectedTracks() {
  QList<int> polysToDelete;
  QList<QTreeWidgetItem *> itemsToDelete = gpsTreeWidget->selectedItems();
  foreach(QTreeWidgetItem *item, itemsToDelete) {
    polysToDelete.push_back(gpsTreeWidget->invisibleRootItem()->
		                                     indexOfChild(item));
  }
  qSort(polysToDelete);
  
  for (int i=polysToDelete.count()-1; i>=0; i--) {
    gpsTreeWidget->invisibleRootItem()->removeChild(gpsTreeWidget->invisibleRootItem()->child(polysToDelete.at(i)));
    GpsPolygon *tmp = polys.at(polysToDelete.at(i));
    mapView->deleteMapPolygon(tmp);
    polys.removeAt(polysToDelete.at(i));
    if (tmp) delete(tmp); // Must delete list items manually
  } 

  updateGpsPositions();
}
