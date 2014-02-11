/*
 * MainWindow.cpp
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

#include <QtGui>
#include "MainWindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QDebug>

//-----------------------------------------------------------------
MainWindow::MainWindow()
{
	setupUi(this);

	this->setWindowTitle("Desktop Capture");
	this->setWindowIcon(QIcon(":/DesktopCapture/DesktopCapture.ico"));
}

//-----------------------------------------------------------------
void MainWindow::open()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open a DXF file"), QDir::currentPath(), tr("(DXF files) *.DXF"));
	if (!fileName.isEmpty())
		loadFile(fileName);
}

//-----------------------------------------------------------------
void MainWindow::about()
{
	QMessageBox::about(this, tr("DesktopSpy Demo"), tr("Demo de capturador de pantallas + webcam."));
}

//-----------------------------------------------------------------
void MainWindow::loadFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("Application"), tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

  qDebug() << "Reading file:" << fileName;

  this->setWindowTitle(QString("Energia Demo - ") + fileName);

  m_filename = fileName;
  parserSelected(true);
}

void MainWindow::parserSelected(bool value)
{
  QAction *origin = qobject_cast<QAction *>(sender());

	if (value == false)
	{
		origin->setChecked(true);
		return;
	}

	if (m_filename.isEmpty())
		return;

	statusBar()->showMessage(tr("File loaded correctly."), 2000);
}

//-----------------------------------------------------------------
bool MainWindow::saveFile(const QString &fileName)
{
	Q_ASSERT(false);

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("Application"), tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return false;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::restoreOverrideCursor();

	statusBar()->showMessage(tr("File saved"), 2000);
	return true;
}
