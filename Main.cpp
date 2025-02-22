/*
    File: Main.cpp
    Created on: 30/05/2015
    Author: Felix de las Pozas Alvarez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include <DesktopCapture.h>

// Qt
#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// NOTE: this is needed to use QPixmaps inside threads,
	// but only on linux because of X11 architecture.
	app.setQuitOnLastWindowClosed(false);

	// allow only one instance
  QSharedMemory guard;
  guard.setKey("DesktopCapture");

  if (!guard.create(1))
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/DesktopCapture/application.ico"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("DesktopCapture is already running!");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    exit(0);
  }

	DesktopCapture desktopCapture;
	desktopCapture.show();

  const auto returnValue = app.exec();
  qDebug() << "Application finished with return value" << returnValue;
  
	return returnValue;
}


