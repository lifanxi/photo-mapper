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

#ifndef _GOOGLEEARTHEXPORT_DIALOG_H
#define _GOOGLEEARTHEXPORT_DIALOG_H

#include "ui_googleEarthExport_dialog.h"
#include <QSettings>

class googleEarthExport_dialog: public QDialog,
                                private Ui::googleEarthExport_dialog {
  Q_OBJECT
  public:
    googleEarthExport_dialog(QWidget *parent, QSettings *);
    bool thumbsEnabled();
    int getThumbnailWidth();
    int getThumbnailHeight();
    bool hyperTextEnabled();
    int getJpegQuality();
    QString getHyperlinkDir();
    QString getExportFilename();
    bool getLaunchGoogleEarth();
	bool getPolysEnabled();
	QRgb getColor();
	bool get3DexportEnabled();
        bool getIconsEnabled();

  private slots:
    void on_geThumbCheckBox_stateChanged(int state);
    void on_geHyperLinksCheckBox_stateChanged(int state);
    void on_geIconsCheckBox_stateChanged(int state);
    void on_filenameButton_clicked(bool checked=false);
    void on_geHyperLinksBrowseButton_clicked(bool checked=false);
	void on_gePolyColorButton_clicked();
	void on_gePolyCheckBox_stateChanged(int state);
	void on_radioButton2d_toggled(bool checked);
	void on_radioButton3d_toggled(bool checked);
	
  private:
    bool tnEnabled;
    bool hyperEnabled;
	bool polysEnabled;
	QRgb polyColor;
	QPalette polyPalette;
	bool export3d;
        bool iconsEnabled;
};

#endif
