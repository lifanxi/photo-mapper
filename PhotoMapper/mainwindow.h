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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include "ui_mainwindow.h"
#include <gps/gpsdata.h>
#include <photo.h>
#include <about_dialog.h>
#include <QSettings>
#include "googleEarthExport_dialog.h"
#include "tag_dialog.h"
#include "previewthread.h"
#include "fileFormatInterface.h"

enum e_stringType {
  EXIF_LOLA,
  EXIF_ALT,
  EXIF_DATE,
  STANDARD
};

class MainWindow: public QMainWindow, private Ui::MainWindow {
  Q_OBJECT
  public:
    MainWindow(QWidget *parent=NULL);

    static double convertDegToDecimal(QString degPosStr);
    static QString convertDecimalToDeg(double value);
    static QString convertDegToExif(QString degPosStr);
    static QString convertString(QString qstr, e_stringType stype);
  public slots:
    void on_actionImport_GPS_data_activated();
    void on_actionQuit_activated();
    void on_imageFileTable_itemSelectionChanged();
    void on_googleEarthButton_clicked();
    void on_tagButton_clicked();
    void on_imageFileTable_cellChanged(int row, int col);
    void on_actionAbout_Copiks_PhotoMapper_activated();
    void deleteImagesClicked();
    void on_actionOnline_help_activated();
    void on_actionImport_images_activated();
    void on_timezoneCombo_currentIndexChanged(QString s);
    void on_timeCorrectionTimeEdit_timeChanged(const QTime t);
    void on_timeCorrectionSignCombo_activated(QString s);
    void on_actionContribute_activated();
    void updateImage(const QImage &img, Photo *pht);
    void removeSelectedTracks();
    void zoomToSelectedPhotos();

  signals:
    void photoUnselected(Photo *);
    void photoSelected(Photo *);

  private:
    void updateGpsPositions();
    QTransform getTransformation(Photo*);
    QList<GpsPolygon*> polys;
    QPixmap previewImage;
    QList<Photo *> photoList;
    bool callbackDisabled;
    AboutDialog *aboutD;
    googleEarthExport_dialog *geDialog;
    tag_dialog *tagDialog;
    float computerTimeZone;
    float selectedTimeZone;
    int timeFineAdjust;
    int timeFineAdjustSign;
    PreviewThread *previewThread;
    void loadPlugins();
    QList <FileFormatInterface*> filePlugins;
    QSettings *settings;
    QSet<int> selectedRows;
};

#endif
