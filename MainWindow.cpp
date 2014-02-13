/*
 * MainWindow.cpp
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

// Project
#include "MainWindow.h"
#include "ProbeResolutionsDialog.h"

// Qt
#include <QtGui>
#include <QDebug>


//-----------------------------------------------------------------
MainWindow::MainWindow()
{
	setupUi(this);

	this->setWindowTitle("Desktop Capture");
	this->setWindowIcon(QIcon(":/DesktopCapture/DesktopCapture.ico"));
	this->showMaximized();

	setupMonitors();
	setupCameraResolutions();

	m_desktopCapture = QPixmap::grabWindow(QApplication::desktop()->winId(), QApplication::desktop()->x(), QApplication::desktop()->y(), QApplication::desktop()->width(), QApplication::desktop()->height());
	updateCapturedImage();
	saveCapture();
}

//-----------------------------------------------------------------
MainWindow::~MainWindow()
{
	if(m_camera.isOpened())
		m_camera.release();
}

//-----------------------------------------------------------------
void MainWindow::resizeEvent(QResizeEvent * /* event */)
{
	QSize scaledSize = m_desktopCapture.size();
	scaledSize.scale(m_screenshotImage->size(), Qt::KeepAspectRatio);
	if (!m_screenshotImage->pixmap() || scaledSize != m_screenshotImage->pixmap()->size())
		updateCapturedImage();
}

//-----------------------------------------------------------------
void MainWindow::setupMonitors()
{
	QDesktopWidget *desktop = QApplication::desktop();
	QStringList monitors;

	for (int i = 0; i < desktop->numScreens(); ++i)
	{
		auto geometry = desktop->screenGeometry(i);
		if (desktop->primaryScreen() == i)
			monitors << QString("Primary Screen (Size: %1x%2 - Position: %3x%4)").arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
		else
			monitors << QString("Additional Screen %1 (Size: %2x%3 - Position: %4x%5)").arg(i).arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
	}
	m_captureMonitorComboBox->insertItems(0, monitors);

	connect(m_captureAllMonitors, SIGNAL(stateChanged(int)), this, SLOT(updateMonitorsComboBox(int)));
}

//-----------------------------------------------------------------
void MainWindow::setupCameraResolutions()
{
	if (!m_camera.open(0))
	{
		m_enableCamera->setChecked(false);
		m_enableCamera->setEnabled(false);
		m_cameraResolutionComboBox->insertItem(0, QString("No cameras detected."));
		m_cameraResolutionComboBox->setEnabled(false);
		m_resolutionsScan->setEnabled(false);
	}
	else
	{
		ProbeResolutionsDialog *dialog = new ProbeResolutionsDialog(this);
		dialog->setWindowIcon(QIcon(":/DesktopCapture/config.ico"));
		dialog->setWindowTitle(QString("Probing camera resolutions..."));
		dialog->exec();

		if (dialog->result() == QDialog::Accepted)
		{
			auto resolutions = dialog->getResolutions();
			QStringList resolutionNames;
			for (auto resolution: resolutions)
				resolutionNames << getResolutionAsString(resolution);

			m_cameraResolutionComboBox->insertItems(0, resolutionNames);
			m_cameraResolutionComboBox->setCurrentIndex(resolutionNames.size() / 2);

			connect(m_enableCamera, SIGNAL(stateChanged(int)), this, SLOT(updateCameraResolutionsComboBox(int)));
		}
		else
		{
			m_enableCamera->setChecked(false);
			m_enableCamera->setEnabled(false);
			m_cameraResolutionComboBox->insertItem(0, QString("No known resolutions."));
			m_cameraResolutionComboBox->setEnabled(false);
			m_resolutionsScan->setEnabled(false);
		}

		delete dialog;
	}
}

//-----------------------------------------------------------------
void MainWindow::updateMonitorsComboBox(int status)
{
	m_captureMonitorComboBox->setEnabled(status == Qt::Unchecked);
}

//-----------------------------------------------------------------
void MainWindow::updateCameraResolutionsComboBox(int status)
{
	bool enabled = (status == Qt::Checked);
	m_cameraResolutionComboBox->setEnabled(enabled);
	m_resolutionsScan->setEnabled(enabled);
}

//-----------------------------------------------------------------
void MainWindow::updateCapturedImage()
{
	m_screenshotImage->setPixmap(m_desktopCapture.scaled(m_screenshotImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

//-----------------------------------------------------------------
void MainWindow::saveCapture()
{
	QString format = "png";
	QString fileName = QDir::currentPath() + tr("/untitled.") + format;

	m_desktopCapture.save(fileName, format.toAscii());
}
