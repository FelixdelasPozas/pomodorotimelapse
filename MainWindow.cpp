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
: m_trayIcon{nullptr}
{
	setupUi(this);

	this->setWindowTitle("Desktop Capture");
	this->setWindowIcon(QIcon(":/DesktopCapture/application.ico"));
	this->showMaximized();

	setupMonitors();
	setupTrayIcon();
	setupCameraResolutions();

	updateCapturedImage();
}

//-----------------------------------------------------------------
MainWindow::~MainWindow()
{
	if(m_camera.isOpened())
		m_camera.release();
}

//-----------------------------------------------------------------
void MainWindow::resizeEvent(QResizeEvent *unused)
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
	connect(m_captureMonitorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCapturedImage()));
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
		m_camera.release();
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
	updateCapturedImage();
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
	QRect captureGeometry;
	if (this->m_captureAllMonitors->isChecked())
		captureGeometry = QApplication::desktop()->geometry();
	else
	{
		int monitorIndex = this->m_captureMonitorComboBox->currentIndex();
		captureGeometry = QApplication::desktop()->screenGeometry(monitorIndex);
	}

	m_desktopCapture = QPixmap::grabWindow(QApplication::desktop()->winId(), captureGeometry.x(), captureGeometry.y(), captureGeometry.width(), captureGeometry.height());
	m_screenshotImage->setPixmap(m_desktopCapture.scaled(m_screenshotImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

//-----------------------------------------------------------------
void MainWindow::saveCapture()
{
	QString format = "png";
	QString fileName = QDir::currentPath() + tr("/DesktopCapture.") + format;

	m_desktopCapture.save(fileName, format.toAscii());
}

//-----------------------------------------------------------------
void MainWindow::setupTrayIcon()
{
	if(!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	qDebug() << QApplication::palette().color(QPalette::Window);

	m_trayIcon = new QSystemTrayIcon(this);
	m_trayIcon->setIcon(QIcon(":/DesktopCapture/application.ico"));
	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activateTrayIcon(QSystemTrayIcon::ActivationReason)));
}

//-----------------------------------------------------------------
void MainWindow::changeEvent(QEvent* e)
{
	switch (e->type())
	{
		case QEvent::WindowStateChange:
			if ((windowState() & Qt::WindowMinimized) && (m_trayIcon != nullptr))
			{
				QMetaObject::invokeMethod(this, "hide", Qt::QueuedConnection);
				m_trayIcon->show();
				e->ignore();
				return;
			}
			break;
		default:
			break;
	}

	QMainWindow::changeEvent(e);
}

//-----------------------------------------------------------------
void MainWindow::activateTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
	if ((reason) && (reason != QSystemTrayIcon::DoubleClick))
		return;

	m_trayIcon->hide();
	show();
	setWindowState(windowState() & (~Qt::WindowMinimized | Qt::WindowActive));
}
