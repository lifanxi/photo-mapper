/*
    PhotoMapper - Tag Photos with GPS data
    Copyright (C) 2007 Christoffer Hallqvist, COPIKS

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

#include <QApplication>
#include <mainwindow.h>
#include <QSplashScreen>
#include <stdlib.h>
#include <QDir>
#include <QPlastiqueStyle>
#include <QNetworkProxy>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.addLibraryPath(app.applicationDirPath()+ QDir::separator() + "plugins"); // jpeg plugin
  app.setStyle(new QPlastiqueStyle);

  QSettings settings(QCoreApplication::applicationDirPath()+"/photomapper.ini",QSettings::IniFormat);

  // Check if manual proxy settings have been added to ini-file.
  if (settings.value("Proxy/proxyenabled",false).toBool()) {
     QNetworkProxy proxy;
     proxy.setType(QNetworkProxy::Socks5Proxy);
     proxy.setHostName(settings.value("Proxy/proxyhost",QString("localhost")).toString());
     proxy.setPort(settings.value("Proxy/proxyport",1080).toInt());
     proxy.setUser(settings.value("Proxy/proxyuser",QString("username")).toString());
     proxy.setPassword(settings.value("Proxy/proxypasswd","password").toString());
     QNetworkProxy::setApplicationProxy(proxy);
  }

  MainWindow mainWin;
  mainWin.show();

  return app.exec();
}
