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

#include "googleEarthExport_dialog.h"
#include <QDir>
#include <QFileDialog>
#include <QColorDialog>


googleEarthExport_dialog::googleEarthExport_dialog(QWidget *parent, 
		                                   QSettings *settings) {
  setupUi(this);
  tnEnabled = true;
  hyperEnabled = true;
  polysEnabled = false;
  export3d = false;
  iconsEnabled = true;

  polyPalette = QPalette();
  polyColor = qRgb(150,50,190);
  polyPalette.setColor(QPalette::Window,QColor(polyColor));
  gePolyColorButton->setPalette(polyPalette);
 
  QString kmzfile = settings->value("GoogleEarth/kmzfile",
		          QDir::toNativeSeparators(QDir::currentPath() + 
		          QDir::separator() + "temp.kmz")).toString();

  QString hyperLinkPath = settings->value("GoogleEarth/hyperlinkpath",
		     QDir::toNativeSeparators(QDir::currentPath())).toString();
		     
  int tnWid = settings->value("GoogleEarth/thumbwidth",400).toInt();
  thumbWidthLineEdit->setText(QString::number(tnWid));
  int tnHgt = settings->value("GoogleEarth/thumbheight",400).toInt();
  thumbHeightLineEdit->setText(QString::number(tnHgt));

  jpegSlider->setValue(settings->value("GoogleEarth/jpegquality",60).toInt());
  gePolyCheckBox->
    setChecked(settings->value("GoogleEarth/polyenabled",true).toBool());
  
  geHyperLinksCheckBox->
    setChecked(settings->value("GoogleEarth/hyperenabled",false).toBool());
  
  bool is2d = settings->value("GoogleEarth/poly2d",true).toBool();  
  if (is2d) {
  	radioButton2d->setChecked(true);
  } else {
    radioButton3d->setChecked(true);
  }

  geThumbCheckBox->
    setChecked(settings->value("GoogleEarth/thumbsenabled",true).toBool());
  geLaunchCheckBox->
    setChecked(settings->value("GoogleEarth/launch",true).toBool());
  geIconsCheckBox->
    setChecked(settings->value("GoogleEarth/iconsenabled",true).toBool());

  geHyperLinksLineEdit->setText(hyperLinkPath);
  filenameLineEdit->setText(kmzfile);
                    
					
}

void googleEarthExport_dialog::on_geThumbCheckBox_stateChanged(int state) {
  tnEnabled = (state!=0);

  thumbWidthLineEdit->setEnabled(tnEnabled);
  thumbHeightLineEdit->setEnabled(tnEnabled);
  geIconsCheckBox->setEnabled(tnEnabled);
  jpegLabel->setEnabled(tnEnabled);
  jpegSlider->setEnabled(tnEnabled);
  percentLabel->setEnabled(tnEnabled);
  widthLabel->setEnabled(tnEnabled);
  heightLabel->setEnabled(tnEnabled);
}

void googleEarthExport_dialog::on_geHyperLinksCheckBox_stateChanged(int state) {
  hyperEnabled = (state!=0);
  urlLabel->setEnabled(hyperEnabled);
  geHyperLinksLineEdit->setEnabled(hyperEnabled);
  geHyperLinksBrowseButton->setEnabled(hyperEnabled);
}

void googleEarthExport_dialog::on_filenameButton_clicked(bool checked) {
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                            filenameLineEdit->text(),
                            tr("Google Earth (*.kmz)"));
  if (fileName != NULL) {
     filenameLineEdit->setText(fileName);
  }
}


void googleEarthExport_dialog::on_geHyperLinksBrowseButton_clicked(bool checked) {
  QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                  QDir::currentPath(),
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);
  if (dir != NULL) {
    geHyperLinksLineEdit->setText(dir);
  }
}

bool googleEarthExport_dialog::thumbsEnabled() {
  return tnEnabled;
}

int googleEarthExport_dialog::getThumbnailWidth() {
  return thumbWidthLineEdit->text().toInt();
}
int googleEarthExport_dialog::getThumbnailHeight() {
  return thumbHeightLineEdit->text().toInt();
}

bool googleEarthExport_dialog::hyperTextEnabled() {
  return hyperEnabled;
}

int googleEarthExport_dialog::getJpegQuality() {
  return jpegSlider->value();
}

QString googleEarthExport_dialog::getHyperlinkDir() {
  return geHyperLinksLineEdit->text();
}

QString googleEarthExport_dialog::getExportFilename() {
  return filenameLineEdit->text();
}

bool googleEarthExport_dialog::getLaunchGoogleEarth() {
  return geLaunchCheckBox->isChecked();
}

void googleEarthExport_dialog::on_gePolyCheckBox_stateChanged(int state) {
  polysEnabled = (state!=0);
  radioButton2d->setChecked(true);
  radioButton2d->setEnabled(polysEnabled);
  radioButton3d->setEnabled(polysEnabled);
  
}

void googleEarthExport_dialog::on_geIconsCheckBox_stateChanged(int state) {
  iconsEnabled = (state!=0);
}

bool googleEarthExport_dialog::getPolysEnabled() {
  return polysEnabled;
}

void googleEarthExport_dialog::on_gePolyColorButton_clicked() {
  polyColor = QColorDialog::getRgba(polyColor,NULL,this);
  polyPalette.setColor(QPalette::Window,QColor(polyColor));
  gePolyColorButton->setPalette(polyPalette);
}

QRgb googleEarthExport_dialog::getColor() {
  return polyColor;
}

void googleEarthExport_dialog::on_radioButton2d_toggled(bool checked) {
}

void googleEarthExport_dialog::on_radioButton3d_toggled(bool checked) {
  export3d = checked;
}

bool googleEarthExport_dialog::get3DexportEnabled() {
  return export3d;
}

bool googleEarthExport_dialog::getIconsEnabled() {
  return iconsEnabled;
}
