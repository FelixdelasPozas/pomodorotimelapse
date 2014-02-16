/*
 * DesktopCapture.cpp
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

// Project
#include "DesktopCapture.h"
#include "ProbeResolutionsDialog.h"
#include "CaptureDesktopThread.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// Qt
#include <QtGui>
#include <QString>
#include <QPixmap>
#include <QSettings>

const QString DesktopCapture::CAPTURED_MONITOR = QString("Captured Desktop Monitor");
const QString DesktopCapture::MONITORS_LIST = QString("Monitor Resolutions");
const QString DesktopCapture::OUTPUT_DIR = QString("Output Directory");
const QString DesktopCapture::CAMERA_ENABLED = QString("Camera Enabled");
const QString DesktopCapture::CAMERA_ANIMATED_TRAY_ENABLED = QString("Camera Animated Tray Icon");
const QString DesktopCapture::CAMERA_RESOLUTIONS = QString("Available Camera Resolutions");
const QString DesktopCapture::ACTIVE_RESOLUTION = QString("Active Resolution");
const QString DesktopCapture::APPLICATION_GEOMETRY = QString("Application Geometry");


//-----------------------------------------------------------------
DesktopCapture::DesktopCapture()
: m_trayIcon{nullptr}
, m_captureThread{nullptr}
{
	setupUi(this);

	QSettings settings("DesktopCapture.ini", QSettings::IniFormat);

	setWindowTitle("Desktop Capture");
	setWindowIcon(QIcon(":/DesktopCapture/application.ico"));

	if (settings.contains(APPLICATION_GEOMETRY))
		restoreGeometry(settings.value(APPLICATION_GEOMETRY).toByteArray());
	else
		showMaximized();

	setupMonitors();
	setupTrayIcon();
	setupCameraResolutions();
	setupCaptureThread();

	QString outputDir;
	if (settings.contains(OUTPUT_DIR))
		outputDir = settings.value(OUTPUT_DIR, QString()).toString();
	else
	{
		outputDir = QDir::currentPath();
		settings.setValue(OUTPUT_DIR, outputDir);
	}
	m_dirEditLabel->setText(outputDir);

	settings.sync();

	connect(m_enableCamera, SIGNAL(stateChanged(int)), this, SLOT(updateCameraResolutionsComboBox(int)), Qt::QueuedConnection);
	connect(m_dirButton, SIGNAL(pressed()), this, SLOT(updateOutputDir()));
	connect(m_cameraResolutionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCameraResolution(int)));
}

//-----------------------------------------------------------------
DesktopCapture::~DesktopCapture()
{
	saveConfiguration();

	if (m_captureThread)
	{
		m_captureThread->abort();
		m_captureThread->wait();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::saveConfiguration()
{
	QSettings settings("DesktopCapture.ini", QSettings::IniFormat);
	int capturedMonitor = (m_captureAllMonitors->isChecked() ? -1 : m_captureMonitorComboBox->currentIndex());

	settings.setValue(CAPTURED_MONITOR, capturedMonitor);

	QStringList monitors;
	for (int i = 0; i < m_captureMonitorComboBox->count(); ++i)
		monitors << m_captureMonitorComboBox->itemText(i);

	settings.setValue(MONITORS_LIST, monitors);
	settings.setValue(OUTPUT_DIR, m_dirEditLabel->text());
	settings.setValue(CAMERA_ENABLED, m_enableCamera->isChecked());
	settings.setValue(CAMERA_ANIMATED_TRAY_ENABLED, m_screenshotAnimateTray->isChecked());
	settings.setValue(CAMERA_RESOLUTIONS, m_cameraResolutionsNames);
	settings.setValue(ACTIVE_RESOLUTION, m_cameraResolutionComboBox->currentIndex());

	if (this->isVisible())
		settings.setValue(APPLICATION_GEOMETRY, saveGeometry());

	settings.sync();
}

//-----------------------------------------------------------------
void DesktopCapture::setupMonitors()
{
	QSettings settings("DesktopCapture.ini", QSettings::IniFormat);

	int capturedMonitor;
	if (settings.contains(CAPTURED_MONITOR))
		capturedMonitor = settings.value(CAPTURED_MONITOR, -1).toInt();
	else
		capturedMonitor = -1;

	m_captureAllMonitors->setChecked(capturedMonitor == -1);

	QStringList monitors;
	if (settings.contains(MONITORS_LIST))
		monitors = settings.value(MONITORS_LIST, QStringList()).toStringList();
	else
	{
		QDesktopWidget *desktop = QApplication::desktop();
		for (int i = 0; i < desktop->numScreens(); ++i)
		{
			auto geometry = desktop->screenGeometry(i);
			if (desktop->primaryScreen() == i)
				monitors << QString("Primary Screen (Size: %1x%2 - Position: %3x%4)").arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
			else
				monitors << QString("Additional Screen %1 (Size: %2x%3 - Position: %4x%5)").arg(i).arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
		}
		settings.setValue(MONITORS_LIST, monitors);
	}
	m_captureMonitorComboBox->insertItems(0, monitors);

	connect(m_captureAllMonitors, SIGNAL(stateChanged(int)), this, SLOT(updateMonitorsCheckBox(int)));
	connect(m_captureMonitorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMonitorsComboBox(int)));
	settings.sync();
}

//-----------------------------------------------------------------
void DesktopCapture::setupCameraResolutions()
{
	if (!m_cameraResolutions.empty())
		return;

	QSettings settings("DesktopCapture.ini", QSettings::IniFormat);

	bool enableCamera;
	if (settings.contains(CAMERA_ENABLED))
		enableCamera = settings.value(CAMERA_ENABLED, true).toBool();
	else
	{
		enableCamera = true;
		settings.setValue(CAMERA_ENABLED, true);
	}
	m_enableCamera->setChecked(enableCamera);

	bool animateScreenshot;
	if (settings.contains(CAMERA_ANIMATED_TRAY_ENABLED))
		animateScreenshot = settings.value(CAMERA_ANIMATED_TRAY_ENABLED, true).toBool();
	else
	{
		animateScreenshot = true;
		settings.setValue(CAMERA_ANIMATED_TRAY_ENABLED, true);
	}
	m_screenshotAnimateTray->setEnabled(animateScreenshot);

	int selectedResolution;
	if (settings.contains(ACTIVE_RESOLUTION))
		selectedResolution = settings.value(ACTIVE_RESOLUTION, 0).toInt();
	else
	{
		selectedResolution = 0;
		settings.setValue(ACTIVE_RESOLUTION, selectedResolution);
	}

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

		if (settings.contains(CAMERA_RESOLUTIONS))
			m_cameraResolutionsNames = settings.value(CAMERA_RESOLUTIONS, QStringList()).toStringList();

		if (!m_cameraResolutionsNames.empty())
		{
			m_cameraResolutionComboBox->insertItems(0, m_cameraResolutionsNames);

			for(auto resolutionString: m_cameraResolutionsNames)
			{
				QStringList parts = resolutionString.split(" ");
				QStringList numbers = parts[0].split("x");

				bool correct;
				int width = numbers[0].toInt(&correct, 10);
				if (!correct)
					continue;

				int height = numbers[1].toInt(&correct, 10);
				if (!correct)
					continue;

				m_cameraResolutions << getResolution(width, height);
			}
		}
		else
		{
			ProbeResolutionsDialog *dialog = new ProbeResolutionsDialog(this);
			dialog->setWindowIcon(QIcon(":/DesktopCapture/config.ico"));
			dialog->setWindowTitle(QString("Probing camera resolutions..."));
			dialog->exec();

			if (dialog->result() == QDialog::Accepted)
			{
				m_cameraResolutions = dialog->getResolutions();

				for (auto resolution: m_cameraResolutions)
					m_cameraResolutionsNames << getResolutionAsString(resolution);

				m_cameraResolutionComboBox->insertItems(0, m_cameraResolutionsNames);
				m_cameraResolutionComboBox->setCurrentIndex(m_cameraResolutionsNames.size() / 2);

				if (m_captureThread)
				{
					m_captureThread->setResolution(m_cameraResolutions.at(selectedResolution));
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
	}
	settings.setValue(CAMERA_RESOLUTIONS, m_cameraResolutionsNames);

	if (selectedResolution <= m_cameraResolutionComboBox->count())
		m_cameraResolutionComboBox->setCurrentIndex(selectedResolution);

	if (m_captureThread)
		m_captureThread->resume();
	settings.sync();
}

//-----------------------------------------------------------------
void DesktopCapture::updateMonitorsCheckBox(int status)
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
void DesktopCapture::updateMonitorsComboBox(int index)
{
	if (m_captureThread)
		m_captureThread->setCaptureMonitor(index);
}

//-----------------------------------------------------------------
void DesktopCapture::updateCameraResolutionsComboBox(int status)
{
	bool enabled = (status == Qt::Checked);

	if (m_cameraResolutions.isEmpty() && enabled)
		setupCameraResolutions();

	m_cameraResolutionComboBox->setEnabled(enabled);

	if (m_captureThread)
		m_captureThread->setCameraEnabled(enabled);
}

//-----------------------------------------------------------------
void DesktopCapture::saveCapture()
{
	QString format = "png";
	QString fileName = QDir::currentPath() + tr("/DesktopCapture.") + format;

	m_desktopCapture.save(fileName, format.toAscii());
}

//-----------------------------------------------------------------
void DesktopCapture::setupTrayIcon()
{
	if(!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	m_trayIcon = new QSystemTrayIcon(this);
	m_trayIcon->setIcon(QIcon(":/DesktopCapture/application.ico"));
	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activateTrayIcon(QSystemTrayIcon::ActivationReason)));
}

//-----------------------------------------------------------------
void DesktopCapture::changeEvent(QEvent* e)
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
void DesktopCapture::activateTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
	if ((reason) && (reason != QSystemTrayIcon::DoubleClick))
		return;

	m_trayIcon->hide();
	show();
	setWindowState(windowState() & (~Qt::WindowMinimized | Qt::WindowActive));
}

//-----------------------------------------------------------------
void DesktopCapture::setupCaptureThread()
{
	int monitor = -1;
	if (!m_captureAllMonitors->isChecked())
		monitor = m_captureMonitorComboBox->currentIndex();

	Resolution resolution{QString(), 0, 0};

	if (m_enableCamera->isChecked() && !m_cameraResolutions.empty())
		resolution = m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex());

	m_captureThread = new CaptureDesktopThread(monitor, resolution, this);

	connect(m_captureThread, SIGNAL(render()), this, SLOT(renderImage()), Qt::QueuedConnection);
	m_captureThread->start(QThread::Priority::NormalPriority);
}

//-----------------------------------------------------------------
void DesktopCapture::renderImage()
{
	QPixmap* pixmap = m_captureThread->getImage();
	m_screenshotImage->setPixmap(pixmap->scaled(m_screenshotImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

//-----------------------------------------------------------------
void DesktopCapture::updateOutputDir()
{
	if (m_captureThread)
		m_captureThread->pause();
	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::currentPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (m_captureThread)
		m_captureThread->resume();

	m_dirEditLabel->setText(dir);
}

//-----------------------------------------------------------------
void DesktopCapture::updateCameraResolution(int status)
{
	if (m_captureThread)
		m_captureThread->setResolution(this->m_cameraResolutions.at(status));
}
