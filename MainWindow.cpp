/*
 * MainWindow.cpp
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

// Project
#include "MainWindow.h"
#include "ProbeResolutionsDialog.h"
#include "CaptureDesktopThread.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// Qt
#include <QtGui>
#include <QDebug>
#include <QPixmap>

//-----------------------------------------------------------------
MainWindow::MainWindow()
: m_trayIcon{nullptr}
, m_captureThread{nullptr}
{
	setupUi(this);

	this->setWindowTitle("Desktop Capture");
	this->setWindowIcon(QIcon(":/DesktopCapture/application.ico"));
	this->showMaximized();

	setupMonitors();
	setupTrayIcon();
	setupCameraResolutions();
	setupCaptureThread();

	connect(m_enableCamera, SIGNAL(stateChanged(int)), this, SLOT(updateCameraResolutionsComboBox(int)), Qt::QueuedConnection);
}

//-----------------------------------------------------------------
MainWindow::~MainWindow()
{
	if (m_captureThread)
	{
		m_captureThread->abort();
		m_captureThread->wait();
	}
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

	connect(m_captureAllMonitors, SIGNAL(stateChanged(int)), this, SLOT(updateMonitorsCheckBox(int)));
	connect(m_captureMonitorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMonitorsComboBox(int)));
}

//-----------------------------------------------------------------
void MainWindow::setupCameraResolutions()
{
	if (!m_cameraResolutions.empty())
		return;

	if (m_captureThread)
		m_captureThread->pause();

	cv::VideoCapture camera;

	if (!camera.open(0))
	{
		m_enableCamera->blockSignals(true);
		m_enableCamera->setChecked(false);
		m_enableCamera->blockSignals(false);
		m_cameraResolutions.clear();
		m_cameraResolutionComboBox->insertItem(0, QString("No cameras detected."));
		m_cameraResolutionComboBox->setEnabled(false);
	}
	else
	{
		camera.release();
		ProbeResolutionsDialog *dialog = new ProbeResolutionsDialog(this);
		dialog->setWindowIcon(QIcon(":/DesktopCapture/config.ico"));
		dialog->setWindowTitle(QString("Probing camera resolutions..."));
		dialog->exec();

		if (dialog->result() == QDialog::Accepted)
		{
			m_cameraResolutions = dialog->getResolutions();
			QStringList resolutionNames;
			for (auto resolution: m_cameraResolutions)
				resolutionNames << getResolutionAsString(resolution);

			m_cameraResolutionComboBox->insertItems(0, resolutionNames);
			m_cameraResolutionComboBox->setCurrentIndex(resolutionNames.size() / 2);

			if (m_captureThread)
			{
				m_captureThread->setResolution(m_cameraResolutions[0]);
				m_captureThread->setCameraEnabled(true);
			}
		}
		else
		{
			m_enableCamera->setChecked(false);
			m_cameraResolutions.clear();
			m_cameraResolutionComboBox->insertItem(0, QString("No known resolutions."));
			m_cameraResolutionComboBox->setEnabled(false);

			if (m_captureThread)
				m_captureThread->setCameraEnabled(false);
		}

		delete dialog;
	}

	if (m_captureThread)
		m_captureThread->resume();
}

//-----------------------------------------------------------------
void MainWindow::updateMonitorsCheckBox(int status)
{
	bool checked = (status == Qt::Checked);
	m_captureMonitorComboBox->setEnabled(!checked);

	if (m_captureThread)
	{
		if (checked)
			m_captureThread->setCaptureMonitor(-1);
		else
			m_captureThread->setCaptureMonitor(m_captureMonitorComboBox->currentIndex());
	}
}

//-----------------------------------------------------------------
void MainWindow::updateMonitorsComboBox(int index)
{
	if (m_captureThread)
		m_captureThread->setCaptureMonitor(index);
}

//-----------------------------------------------------------------
void MainWindow::updateCameraResolutionsComboBox(int status)
{
	bool enabled = (status == Qt::Checked);

	if (m_cameraResolutions.isEmpty() && enabled)
		setupCameraResolutions();

	m_cameraResolutionComboBox->setEnabled(enabled);
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

//-----------------------------------------------------------------
void MainWindow::setupCaptureThread()
{
	int monitor = -1;
	if (!m_captureAllMonitors->isChecked())
		monitor = m_captureMonitorComboBox->currentIndex();

	Resolution resolution{QString(), 0, 0};

	if (m_enableCamera->isChecked())
		resolution = m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex());

	m_captureThread = new CaptureDesktopThread(monitor, resolution, this);

	connect(m_captureThread, SIGNAL(render()), this, SLOT(renderImage()), Qt::QueuedConnection);
	m_captureThread->start(QThread::Priority::NormalPriority);
}

//-----------------------------------------------------------------
void MainWindow::renderImage()
{
	QImage *image = m_captureThread->getImage();
	QPixmap pixmap = QPixmap::fromImage(*image);
	m_screenshotImage->setPixmap(pixmap.scaled(m_screenshotImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
