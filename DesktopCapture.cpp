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
#include "PomodoroStatistics.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// Qt
#include <QtGui>
#include <QString>
#include <QPixmap>
#include <QDebug>
#include <QSettings>

const QString DesktopCapture::CAPTURE_TIME = QString("Time Between Captures");
const QString DesktopCapture::CAPTURE_ENABLED = QString("Enable Desktop Capture");
const QString DesktopCapture::CAPTURE_VIDEO = QString("Capture Video");
const QString DesktopCapture::CAPTURE_VIDEO_FPS = QString("Capture Video FPS");
const QString DesktopCapture::CAPTURED_MONITOR = QString("Captured Desktop Monitor");
const QString DesktopCapture::MONITORS_LIST = QString("Monitor Resolutions");
const QString DesktopCapture::OUTPUT_DIR = QString("Output Directory");
const QString DesktopCapture::APPLICATION_GEOMETRY = QString("Application Geometry");
const QString DesktopCapture::APPLICATION_STATE = QString("Application State");
const QString DesktopCapture::CAMERA_ENABLED = QString("Camera Enabled");
const QString DesktopCapture::CAMERA_ANIMATED_TRAY_ENABLED = QString("Camera Animated Tray Icon");
const QString DesktopCapture::CAMERA_RESOLUTIONS = QString("Available Camera Resolutions");
const QString DesktopCapture::CAMERA_ACTIVE_RESOLUTION = QString("Active Resolution");
const QString DesktopCapture::CAMERA_OVERLAY_POSITION = QString("Camera Overlay Position");
const QString DesktopCapture::CAMERA_OVERLAY_COMPOSITION_MODE = QString("Camera Overlay Composition Mode");
const QString DesktopCapture::CAMERA_OVERLAY_FIXED_POSITION = QString("Camera Overlay Fixed Position");
const QString DesktopCapture::POMODORO_TIME = QString("Pomodoro Time");
const QString DesktopCapture::POMODORO_SHORT_BREAK_TIME = QString("Short Break Time");
const QString DesktopCapture::POMODORO_LONG_BREAK_TIME = QString("Long Break Time");
const QString DesktopCapture::POMODOROS_BEFORE_BREAK = QString("Pomodoros Before A Long Break");
const QString DesktopCapture::POMODOROS_ANIMATED_TRAY_ENABLED = QString("Pomodoro Animated Tray Icon");
const QString DesktopCapture::POMODOROS_USE_SOUNDS = QString("Pomodoro Use Sounds");
const QString DesktopCapture::POMODORO_ENABLED = QString("Enable Pomodoro");
const QString DesktopCapture::POMODOROS_CONTINUOUS_TICTAC = QString("Continuous Tic-Tac");
const QString DesktopCapture::POMODOROS_SESSION_NUMBER = QString("Pomodoros In Session");
const QString DesktopCapture::POMODOROS_LAST_TASK = QString("Last task");
const QString DesktopCapture::POMODOROS_OVERLAY = QString("Overlay Pomodoro Statistics In Capture");
const QString DesktopCapture::POMODOROS_OVERLAY_POSITION = QString("Pomodoro Overlay Position");

//-----------------------------------------------------------------
DesktopCapture::DesktopCapture()
: m_trayIcon{nullptr}
, m_captureThread{nullptr}
, m_secuentialNumber{0}
, m_started{false}
, m_statisticsDialog{nullptr}
, m_menuPause{nullptr}
, m_menuShowStats{nullptr}
, m_menuStopCapture{nullptr}
, m_menuChangeTask{nullptr}
, m_menuQuit{nullptr}
, m_paused{false}
{
	setupUi(this);

	setWindowTitle("Desktop Capture");
	setWindowIcon(QIcon(":/DesktopCapture/application.ico"));

	loadConfiguration();
	setupTrayIcon();
	setupCameraResolutions();

	if (m_captureGroupBox->isChecked())
		setupCaptureThread();

	connect(m_cameraEnabled, SIGNAL(stateChanged(int)), this, SLOT(updateCameraResolutionsComboBox(int)));
	connect(m_dirButton, SIGNAL(pressed()), this, SLOT(updateOutputDir()));
	connect(m_cameraResolutionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCameraResolution(int)));
	connect(m_compositionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCameraCompositionMode(int)));
	connect(m_startButton, SIGNAL(pressed()), this, SLOT(startCapture()));
	connect(m_captureGroupBox, SIGNAL(clicked(bool)), this, SLOT(updateCaptureDesktop(bool)));
	connect(m_pomodoroGroupBox, SIGNAL(clicked(bool)), this, SLOT(updatePomodoro(bool)));
	connect(m_cameraPositionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCameraPositionComboBox(int)));
	connect(m_continuousTicTac, SIGNAL(stateChanged(int)), this, SLOT(updateContinuousTicTac(int)));
	connect(m_pomodoroUseSounds, SIGNAL(stateChanged(int)), this, SLOT(updateUseSounds(int)));
	connect(m_captureAllMonitors, SIGNAL(stateChanged(int)), this, SLOT(updateMonitorsCheckBox(int)));
	connect(m_captureMonitorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMonitorsComboBox(int)));
	connect(m_taskEditButton, SIGNAL(pressed()), this, SLOT(updateTaskName()));
	connect(m_overlayStats, SIGNAL(stateChanged(int)), this, SLOT(updatePomodoroOverlay(int)));
	connect(m_pomodorosBreakNumber, SIGNAL(valueChanged(int)), this, SLOT(updatePomodoroValues()));
	connect(m_pomodorosNumber, SIGNAL(valueChanged(int)), this, SLOT(updatePomodoroValues()));

	m_screenshotImage->installEventFilter(this);

	if (m_screenshotImage->underMouse() && m_captureThread)
		m_captureThread->setPaintFrame(true);
}

//-----------------------------------------------------------------
DesktopCapture::~DesktopCapture()
{
	if (m_captureThread)
	{
		if(m_started)
			delete m_vp8_interface;

		m_captureThread->abort();
		m_captureThread->resume();
		m_captureThread->wait();
	}

	delete m_captureThread;

	saveConfiguration();
}

//-----------------------------------------------------------------
void DesktopCapture::loadConfiguration()
{
	QSettings settings("DesktopCapture.ini", QSettings::IniFormat);

	if (settings.contains(APPLICATION_GEOMETRY))
		restoreGeometry(settings.value(APPLICATION_GEOMETRY).toByteArray());
	else
		showMaximized();

	if (settings.contains(APPLICATION_STATE))
		restoreState(settings.value(APPLICATION_STATE).toByteArray());

	int capturedMonitor;
	if (settings.contains(CAPTURED_MONITOR))
		capturedMonitor = settings.value(CAPTURED_MONITOR, -1).toInt();
	else
	{
		capturedMonitor = -1;
		settings.setValue(CAPTURED_MONITOR, capturedMonitor);
	}
	m_captureAllMonitors->setChecked(capturedMonitor == -1);
	m_captureMonitorComboBox->setEnabled(!this->m_captureAllMonitors->isChecked());

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

	QPoint position(0,0);
	if (settings.contains(CAMERA_OVERLAY_POSITION))
		position = settings.value(CAMERA_OVERLAY_POSITION, QPoint(0,0)).toPoint();
	else
		settings.setValue(CAMERA_OVERLAY_POSITION, position);
	m_PIPposition = position;

	int modeIndex;
	if (settings.contains(CAMERA_OVERLAY_COMPOSITION_MODE))
		modeIndex = settings.value(CAMERA_OVERLAY_COMPOSITION_MODE, 0).toInt();
	else
		modeIndex = 0;
	m_compositionMode = CaptureDesktopThread::COMPOSITION_MODES.at(modeIndex);
	m_compositionComboBox->insertItems(0, CaptureDesktopThread::COMPOSITION_MODES_NAMES);
	m_compositionComboBox->setCurrentIndex(modeIndex);

	bool pomodoroEnabled;
	if (settings.contains(POMODORO_ENABLED))
		pomodoroEnabled = settings.value(POMODORO_ENABLED, true).toBool();
	else
	{
		pomodoroEnabled = true;
		settings.setValue(POMODORO_ENABLED, pomodoroEnabled);
	}
	m_pomodoroGroupBox->setChecked(pomodoroEnabled);

	QTime pomodoroTime;
	if (settings.contains(POMODORO_TIME))
		pomodoroTime = settings.value(POMODORO_TIME).toTime();
	else
	{
		pomodoroTime = QTime(0,25,0);
		settings.setValue(POMODORO_TIME, pomodoroTime);
	}
	m_pomodoroTime->setTime(pomodoroTime);
	m_pomodoro.setPomodoroTime(pomodoroTime);

	QTime shortBreak;
	if (settings.contains(POMODORO_SHORT_BREAK_TIME))
		shortBreak = settings.value(POMODORO_SHORT_BREAK_TIME, QTime(0,5,0)).toTime();
	else
	{
		shortBreak = QTime(0,5,0);
		settings.setValue(POMODORO_SHORT_BREAK_TIME, shortBreak);
	}
	m_shortBreakTime->setTime(shortBreak);
	m_pomodoro.setShortBreakTime(shortBreak);

	QTime longBreak;
	if (settings.contains(POMODORO_LONG_BREAK_TIME))
		longBreak = settings.value(POMODORO_LONG_BREAK_TIME, QTime(0,15,0)).toTime();
	else
	{
		longBreak = QTime(0,15,0);
		settings.setValue(POMODORO_LONG_BREAK_TIME, longBreak);
	}
	m_longBreakTime->setTime(longBreak);
	m_pomodoro.setLongBreakTime(longBreak);

	int pomodorosBeforeBreak;
	if (settings.contains(POMODOROS_BEFORE_BREAK))
		pomodorosBeforeBreak = settings.value(POMODOROS_BEFORE_BREAK, 4).toInt();
	else
	{
		pomodorosBeforeBreak = 4;
		settings.setValue(POMODOROS_BEFORE_BREAK, pomodorosBeforeBreak);
	}
	m_pomodorosBreakNumber->setValue(pomodorosBeforeBreak);
	m_pomodoro.setPomodorosBeforeBreak(pomodorosBeforeBreak);

	bool animatePomodoro;
	if (settings.contains(POMODOROS_ANIMATED_TRAY_ENABLED))
		animatePomodoro = settings.value(POMODOROS_ANIMATED_TRAY_ENABLED, true).toBool();
	else
	{
		animatePomodoro = true;
		settings.setValue(POMODOROS_ANIMATED_TRAY_ENABLED, animatePomodoro);
	}
	m_pomodoroAnimateTray->setChecked(animatePomodoro);

	bool pomodoroUseSounds;
	if (settings.contains(POMODOROS_USE_SOUNDS))
		pomodoroUseSounds = settings.value(POMODOROS_USE_SOUNDS, true).toBool();
	else
	{
		pomodoroUseSounds = true;
		settings.setValue(POMODOROS_USE_SOUNDS, pomodoroUseSounds);
	}
	m_pomodoroUseSounds->setChecked(pomodoroUseSounds);
	m_pomodoro.setUseSounds(pomodoroUseSounds);

	bool pomodoroContinuousTicTac;
	if (settings.contains(POMODOROS_CONTINUOUS_TICTAC))
		pomodoroContinuousTicTac = settings.value(POMODOROS_CONTINUOUS_TICTAC, false).toBool();
	else
	{
		pomodoroContinuousTicTac = false;
		settings.setValue(POMODOROS_CONTINUOUS_TICTAC, pomodoroContinuousTicTac);
	}
	m_continuousTicTac->setChecked(pomodoroContinuousTicTac);
	m_continuousTicTac->setEnabled(pomodoroUseSounds && this->m_pomodoroGroupBox->isChecked());
	m_pomodoro.setContinuousTicTac(pomodoroContinuousTicTac);

	unsigned int pomodorosInSession;
	if (settings.contains(POMODOROS_SESSION_NUMBER))
		pomodorosInSession = settings.value(POMODOROS_SESSION_NUMBER, 12).toUInt();
	else
	{
		pomodorosInSession = 12;
		settings.setValue(POMODOROS_SESSION_NUMBER, pomodorosInSession);
	}
	m_pomodorosNumber->setValue(pomodorosInSession);
	m_pomodoro.setSessionPodomodos(pomodorosInSession);

	QString task;
	if (settings.contains(POMODOROS_LAST_TASK))
		task = settings.value(POMODOROS_LAST_TASK, QString("Undefined task")).toString();
	else
	{
		task = QString("Undefined task");
		settings.setValue(POMODOROS_LAST_TASK, task);
	}
	m_pomodoroTask->setText(task);
	m_pomodoro.setTask(task);

	bool overlayPomodoro;
	if (settings.contains(POMODOROS_OVERLAY))
		overlayPomodoro = settings.value(POMODOROS_OVERLAY, true).toBool();
	else
	{
		overlayPomodoro = true;
		settings.setValue(POMODOROS_OVERLAY, true);
	}
	m_overlayStats->setChecked(overlayPomodoro);
	m_overlayStats->setEnabled(m_pomodoroGroupBox->isChecked());

	if (settings.contains(POMODOROS_OVERLAY_POSITION))
		m_statsPosition = settings.value(POMODOROS_OVERLAY_POSITION, QPoint(0,0)).toPoint();
	else
	{
		m_statsPosition = QPoint(0,0);
		settings.setValue(POMODOROS_OVERLAY_POSITION, m_statsPosition);
	}

	QTime timeBetweenCaptures;
	if (settings.contains(CAPTURE_TIME))
		timeBetweenCaptures = settings.value(CAPTURE_TIME, QTime(0,0,30)).toTime();
	else
	{
		timeBetweenCaptures = QTime(0,0,30);
		settings.setValue(CAPTURE_TIME, timeBetweenCaptures);
	}
	m_screeshotTime->setTime(timeBetweenCaptures);

	bool captureVideo;
	if (settings.contains(CAPTURE_VIDEO))
		captureVideo = settings.value(CAPTURE_VIDEO, true).toBool();
	else
	{
		captureVideo = true;
		settings.setValue(CAPTURE_VIDEO, captureVideo);
	}
	m_captureVideo->setChecked(captureVideo);

	int videoFps;
	if (settings.contains(CAPTURE_VIDEO_FPS))
		videoFps = settings.value(CAPTURE_VIDEO_FPS, 15).toInt();
	else
	{
		videoFps = 15;
		settings.setValue(CAPTURE_VIDEO_FPS, videoFps);
	}
	m_fps->setEnabled(captureVideo);
	m_fps->setValue(videoFps);

	QString outputDir;
	if (settings.contains(OUTPUT_DIR))
		outputDir = settings.value(OUTPUT_DIR, QString()).toString();
	else
	{
		outputDir = QDir::currentPath();
		settings.setValue(OUTPUT_DIR, outputDir);
	}
	m_dirEditLabel->setText(outputDir);

	bool captureEnabled;
	if (settings.contains(CAPTURE_ENABLED))
		captureEnabled = settings.value(CAPTURE_ENABLED, true).toBool();
	else
	{
		captureEnabled = true;
		settings.setValue(CAPTURE_ENABLED, true);
	}
	m_captureGroupBox->setChecked(captureEnabled);

	int cameraPosition;
	if (settings.contains(CAMERA_OVERLAY_FIXED_POSITION))
		cameraPosition = settings.value(CAMERA_OVERLAY_FIXED_POSITION, 0).toInt();
	else
	{
		cameraPosition = 0;
		settings.setValue(CAMERA_OVERLAY_FIXED_POSITION, cameraPosition);
	}
	m_cameraPositionComboBox->insertItems(0, CaptureDesktopThread::POSITION_NAMES);
	m_cameraPositionComboBox->setCurrentIndex(cameraPosition);

	settings.sync();
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
	settings.setValue(CAMERA_ENABLED, m_cameraEnabled->isChecked());
	settings.setValue(CAMERA_ANIMATED_TRAY_ENABLED, m_screenshotAnimateTray->isChecked());
	settings.setValue(CAMERA_RESOLUTIONS, m_cameraResolutionsNames);
	settings.setValue(CAMERA_ACTIVE_RESOLUTION, m_cameraResolutionComboBox->currentIndex());
	settings.setValue(CAMERA_OVERLAY_POSITION, m_PIPposition);
	settings.setValue(CAMERA_OVERLAY_COMPOSITION_MODE, m_compositionComboBox->currentIndex());
  settings.setValue(APPLICATION_GEOMETRY, saveGeometry());
  settings.setValue(APPLICATION_STATE, saveState());
  settings.setValue(POMODORO_ENABLED, m_pomodoroGroupBox->isChecked());
	settings.setValue(POMODORO_TIME, m_pomodoroTime->time());
	settings.setValue(POMODORO_SHORT_BREAK_TIME, m_shortBreakTime->time());
	settings.setValue(POMODORO_LONG_BREAK_TIME, m_longBreakTime->time());
	settings.setValue(POMODOROS_BEFORE_BREAK, m_pomodorosBreakNumber->value());
  settings.setValue(POMODOROS_ANIMATED_TRAY_ENABLED, m_pomodoroAnimateTray->isChecked());
  settings.setValue(POMODOROS_USE_SOUNDS, m_pomodoroUseSounds->isChecked());
  settings.setValue(POMODOROS_CONTINUOUS_TICTAC, m_continuousTicTac->isChecked());
	settings.setValue(POMODOROS_SESSION_NUMBER, m_pomodorosNumber->value());
  settings.setValue(CAPTURE_TIME, m_screeshotTime->time());
  settings.setValue(CAPTURE_ENABLED, m_captureGroupBox->isChecked());
  settings.setValue(CAPTURE_VIDEO, m_captureVideo->isChecked());
  settings.setValue(CAPTURE_VIDEO_FPS, m_fps->value());
  settings.setValue(CAMERA_OVERLAY_FIXED_POSITION, m_cameraPositionComboBox->currentIndex());
  settings.setValue(POMODOROS_LAST_TASK, m_pomodoroTask->text());
  settings.setValue(POMODOROS_OVERLAY, m_overlayStats->isChecked());
  settings.setValue(POMODOROS_OVERLAY_POSITION, m_statsPosition);

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
	m_cameraEnabled->setChecked(enableCamera);

	bool animateScreenshot;
	if (settings.contains(CAMERA_ANIMATED_TRAY_ENABLED))
		animateScreenshot = settings.value(CAMERA_ANIMATED_TRAY_ENABLED, true).toBool();
	else
	{
		animateScreenshot = true;
		settings.setValue(CAMERA_ANIMATED_TRAY_ENABLED, true);
	}
	m_screenshotAnimateTray->setEnabled(animateScreenshot && m_captureGroupBox->isChecked());

	int selectedResolution;
	if (settings.contains(CAMERA_ACTIVE_RESOLUTION))
		selectedResolution = settings.value(CAMERA_ACTIVE_RESOLUTION, 0).toInt();
	else
	{
		selectedResolution = 0;
		settings.setValue(CAMERA_ACTIVE_RESOLUTION, selectedResolution);
	}

	if (m_captureThread)
		m_captureThread->pause();

	cv::VideoCapture camera;
	if (!camera.open(0))
	{
		m_cameraEnabled->blockSignals(true);
		m_cameraEnabled->setChecked(false);
		m_cameraEnabled->blockSignals(false);
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
				m_cameraEnabled->setChecked(false);
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

	if (m_cameraEnabled)
		m_cameraPositionComboBox->setCurrentIndex(0);

	if (m_captureThread)
	{
		if (checked)
			m_captureThread->setCaptureMonitor(-1);
		else
		{
			if (m_cameraEnabled)
			{
				computeNewPIPPosition();
				computeNewStatsPosition();
				m_captureThread->setCameraOverlayPosition(m_PIPposition);
				m_captureThread->setStatsOverlayPosition(m_statsPosition);
			}
			m_captureThread->setCaptureMonitor(m_captureMonitorComboBox->currentIndex());
		}
	}
}

//-----------------------------------------------------------------
void DesktopCapture::updateMonitorsComboBox(int index)
{
	if (m_captureThread)
	{
		m_captureThread->setCaptureMonitor(index);

		if (m_cameraEnabled)
		{
			computeNewPIPPosition();
			m_captureThread->setCameraOverlayPosition(m_PIPposition);
			m_cameraPositionComboBox->setCurrentIndex(0);
		}
	}

}

//-----------------------------------------------------------------
void DesktopCapture::updateCameraResolutionsComboBox(int status)
{
	bool enabled = (status == Qt::Checked);

	if (m_cameraResolutions.isEmpty() && enabled)
		setupCameraResolutions();

	m_cameraResolutionComboBox->setEnabled(enabled);
	m_cameraPositionComboBox->setEnabled(enabled);

	if (m_captureThread)
		m_captureThread->setCameraEnabled(enabled);
}

//-----------------------------------------------------------------
void DesktopCapture::saveCapture(QPixmap *capture)
{
	QString format("png");
	QString fileName = m_dirEditLabel->text() + tr("/DesktopCapture_") + QString("%1").arg(m_secuentialNumber,4,'d',0,'0') + QString(".") + format;
	capture->save(fileName, format.toAscii(), 0);
}

//-----------------------------------------------------------------
void DesktopCapture::setupTrayIcon()
{
	if(!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	QMenu *menu = new QMenu(this);
	m_trayIcon = new QSystemTrayIcon(this);
	m_trayIcon->setIcon(QIcon(":/DesktopCapture/application.ico"));
	m_trayIcon->setContextMenu(menu);
	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activateTrayIcon(QSystemTrayIcon::ActivationReason)));

	m_menuPause = new QAction(QString("Pause pomodoro"), menu);
	connect(m_menuPause, SIGNAL(triggered()), this, SLOT(pausePomodoro()), Qt::QueuedConnection);

	m_menuStopCapture = new QAction(QString("Stop"), menu);
	connect(m_menuStopCapture, SIGNAL(triggered()), this, SLOT(stopCaptureAction()), Qt::QueuedConnection);

	m_menuShowStats = new QAction(QString("Show statistics..."), menu);
	connect(m_menuShowStats, SIGNAL(triggered()), this, SLOT(showStatistics()), Qt::QueuedConnection);

	m_menuChangeTask = new QAction(QString("Change task..."), menu);
	connect(m_menuChangeTask, SIGNAL(triggered()), this, SLOT(changeTask()), Qt::QueuedConnection);

	m_menuQuit = new QAction(QString("Exit application"), menu);
	connect(m_menuQuit, SIGNAL(triggered()), this, SLOT(quitApplication()), Qt::QueuedConnection);
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

				if (m_captureThread != nullptr)
					m_captureThread->pause();
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

	if(!m_started)
	{
		m_trayIcon->hide();

		if (m_captureThread != nullptr)
			m_captureThread->resume();

		show();
		setWindowState(windowState() & (~Qt::WindowMinimized | Qt::WindowActive));
	}
	else
	{
		if(m_pomodoroGroupBox->isChecked() && (m_pomodoro.status() != Pomodoro::Status::Stopped))
		{
			m_paused = false;
			showStatistics();
		}
		else
		{
			stopCaptureAction();
		}
	}
}

//-----------------------------------------------------------------
void DesktopCapture::setupCaptureThread()
{
	int monitor = -1;
	if (!m_captureAllMonitors->isChecked())
		monitor = m_captureMonitorComboBox->currentIndex();

	Resolution resolution{QString(), 0, 0};

	if (m_cameraEnabled->isChecked() && !m_cameraResolutions.empty())
		resolution = m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex());

	m_captureThread = new CaptureDesktopThread(monitor, resolution, m_PIPposition, m_statsPosition, m_compositionMode, this);

	if (m_pomodoroGroupBox->isChecked())
		m_captureThread->setPomodoro(&m_pomodoro);

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

	QDir dir(m_dirEditLabel->text());
	if (!dir.exists())
		m_dirEditLabel->setText(QDir::currentPath());

	QString dirText = QFileDialog::getExistingDirectory(this, tr("Open Directory"), m_dirEditLabel->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	m_dirEditLabel->setText(dirText);

	if (m_captureThread)
		m_captureThread->resume();
}

//-----------------------------------------------------------------
void DesktopCapture::updateCameraResolution(int status)
{
	if (m_captureThread)
	{
		m_captureThread->setResolution(m_cameraResolutions.at(status));
		computeNewPIPPosition();
		m_captureThread->setCameraOverlayPosition(m_PIPposition);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::updateCameraCompositionMode(int status)
{
	m_compositionMode = CaptureDesktopThread::COMPOSITION_MODES.at(status);
	if (m_captureThread)
		m_captureThread->setCameraOverlayCompositionMode(m_compositionMode);
}

//-----------------------------------------------------------------
bool DesktopCapture::eventFilter(QObject *object, QEvent *event)
{
	if (!m_captureThread || !m_cameraEnabled->isChecked() || !m_screenshotImage->pixmap())
		return object->eventFilter(object, event);

	static bool insideCamera = false;
	static bool insideStats = false;
	static bool drag = false;

	static QPoint dragPoint = QPoint(0, 0);
	QMouseEvent *me = static_cast<QMouseEvent *>(event);
	Resolution cameraResolution = m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex());

	QLabel *label = qobject_cast<QLabel *>(object);
	if (!label)
		return false;

	switch (event->type())
	{
		case QEvent::Enter:
			if (m_captureThread)
				m_captureThread->setPaintFrame(true);
			break;
		case QEvent::Leave:
			if (m_captureThread)
				m_captureThread->setPaintFrame(false);
			break;
		case QEvent::MouseButtonPress:
			if (me->button() == Qt::LeftButton)
			{
				QRect geometry;
				if (!m_captureAllMonitors->isChecked())
					geometry = QApplication::desktop()->screenGeometry(m_captureMonitorComboBox->currentIndex());
				else
					geometry = QApplication::desktop()->geometry();

				QSize unusedSpace = m_screenshotImage->size() - m_screenshotImage->pixmap()->size();
				double ratio = geometry.width() / static_cast<double>(m_screenshotImage->pixmap()->size().width());
				QPoint mappedPoint = QPoint((me->pos().x() - unusedSpace.width()/2) * ratio, (me->pos().y() - unusedSpace.height()/2) * ratio);

				if ( (m_PIPposition.x() > mappedPoint.x()) ||
		         (m_PIPposition.y() > mappedPoint.y()) ||
		         (m_PIPposition.x()+cameraResolution.width < mappedPoint.x()) ||
		         (m_PIPposition.y()+cameraResolution.height < mappedPoint.y()) )
				{
					insideCamera = false;
				}
				else
					insideCamera = true;

				int height = 15 * ((2 * m_pomodoro.getPomodorosInSession() - 1) + (m_pomodoro.getPomodorosInSession() / m_pomodoro.getPomodorosBeforeBreak()) - ((m_pomodoro.getPomodorosInSession() % m_pomodoro.getPomodorosBeforeBreak() == 0) ? 1 : 0));
				if ( (m_statsPosition.x() > mappedPoint.x()) ||
		         (m_statsPosition.y() > mappedPoint.y()) ||
		         (m_statsPosition.x()+250 < mappedPoint.x()) ||
		         (m_statsPosition.y()+height < mappedPoint.y()) )
				{
					insideStats = false;
				}
				else
					insideStats = true;

				if(!insideCamera && !insideStats)
					break;

				m_cameraPositionComboBox->setCurrentIndex(0);
				drag = true;
				dragPoint = me->pos();
			}
			break;
		case QEvent::MouseButtonRelease:
			if ((me->button() == Qt::LeftButton) && drag && (insideCamera || insideStats))
			{
				drag = false;
				if(insideCamera)
					m_captureThread->setCameraOverlayPosition(computeNewPIPPosition(dragPoint, me->pos()));
				else
					if(insideStats)
						m_captureThread->setStatsOverlayPosition(computeNewStatsPosition(dragPoint, me->pos()));

				dragPoint = me->pos();
				insideCamera = false;
				insideStats = false;
			}
			break;
		case QEvent::MouseMove:
			if (drag && (insideCamera || insideStats))
			{
				if(insideCamera)
					m_captureThread->setCameraOverlayPosition(computeNewPIPPosition(dragPoint, me->pos()));
				else
					if(insideStats)
						m_captureThread->setStatsOverlayPosition(computeNewStatsPosition(dragPoint, me->pos()));

				dragPoint = me->pos();
			}
			break;
		default:
			break;
	}

	return object->eventFilter(object, event);
}

//-----------------------------------------------------------------
QPoint DesktopCapture::computeNewPIPPosition(const QPoint &dragPoint, const QPoint &point)
{
	QSize imageGeometry = m_screenshotImage->pixmap()->size();
	QRect geometry;
	if (!m_captureAllMonitors->isChecked())
		geometry = QApplication::desktop()->screenGeometry(m_captureMonitorComboBox->currentIndex());
	else
		geometry = QApplication::desktop()->geometry();

	double ratioX = static_cast<double>(geometry.width()) / static_cast<double>(imageGeometry.width());
	double ratioY = static_cast<double>(geometry.height()) / static_cast<double>(imageGeometry.height());

	Resolution cameraResolution = m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex());

	int dx = (point.x() - dragPoint.x())*ratioX;
	int dy = (point.y() - dragPoint.y())*ratioY;
	int xLimit = geometry.width() - cameraResolution.width;
	int yLimit = geometry.height() - cameraResolution.height;

	m_PIPposition.setX(m_PIPposition.x() + dx);
	m_PIPposition.setY(m_PIPposition.y() + dy);

	if (m_PIPposition.x() < 0)
		m_PIPposition.setX(0);

	if (m_PIPposition.x() > xLimit)
		m_PIPposition.setX(xLimit);

	if (m_PIPposition.y() < 0)
		m_PIPposition.setY(0);

	if (m_PIPposition.y() > yLimit)
		m_PIPposition.setY(yLimit);

	return m_PIPposition;
}

//-----------------------------------------------------------------
QPoint DesktopCapture::computeNewStatsPosition(const QPoint &dragPoint, const QPoint &point)
{
	QSize imageGeometry = m_screenshotImage->pixmap()->size();
	QRect geometry;
	if (!m_captureAllMonitors->isChecked())
		geometry = QApplication::desktop()->screenGeometry(m_captureMonitorComboBox->currentIndex());
	else
		geometry = QApplication::desktop()->geometry();

	double ratioX = static_cast<double>(geometry.width()) / static_cast<double>(imageGeometry.width());
	double ratioY = static_cast<double>(geometry.height()) / static_cast<double>(imageGeometry.height());

	int height = 15 * ((2 * m_pomodoro.getPomodorosInSession() - 1) + (m_pomodoro.getPomodorosInSession() / m_pomodoro.getPomodorosBeforeBreak()) - ((m_pomodoro.getPomodorosInSession() % m_pomodoro.getPomodorosBeforeBreak() == 0) ? 1 : 0));
	int dx = (point.x() - dragPoint.x())*ratioX;
	int dy = (point.y() - dragPoint.y())*ratioY;
	int xLimit = geometry.width() - 250;
	int yLimit = geometry.height() - height;

	m_statsPosition.setX(m_statsPosition.x() + dx);
	m_statsPosition.setY(m_statsPosition.y() + dy);

	if (m_statsPosition.x() < 0)
		m_statsPosition.setX(0);

	if (m_statsPosition.x() > xLimit)
		m_statsPosition.setX(xLimit);

	if (m_statsPosition.y() < 0)
		m_statsPosition.setY(0);

	if (m_statsPosition.y() > yLimit)
		m_statsPosition.setY(yLimit);

	return m_statsPosition;
}

//-----------------------------------------------------------------
void DesktopCapture::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
  QCoreApplication::quit();
}

//-----------------------------------------------------------------
void DesktopCapture::startCapture()
{
	if (!m_captureGroupBox->isChecked() && !m_pomodoroGroupBox->isChecked())
		return;

	QString trayMessage;
	m_started = true;

	if (m_captureGroupBox->isChecked())
	{
		Q_ASSERT(m_captureThread != nullptr);

		if (!m_captureThread->isPaused())
			m_captureThread->pause();

		auto time = m_screeshotTime->time();
		int ms = time.second() * 1000 + time.minute() * 1000 * 60 + time.hour() * 60 * 60 * 1000 + time.msec();

		m_timer.setInterval(ms);
		m_timer.setSingleShot(false);
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(capture()), Qt::QueuedConnection);

		QString message("Capture interval set to");
		if (time.hour() != 0)
		{
			message += QString(" %1 hour").arg(time.hour());

			if (time.hour() > 1)
				message += QString("s");
		}

		if (time.minute() != 0)
		{
			message += QString(" %1 minute").arg(time.minute());

			if (time.minute() > 1)
				message += QString("s");
		}

		if (time.second() != 0)
		{
			message += QString(" %1 second").arg(time.second());

			if (time.second() > 1)
				message += QString("s");
		}

		message += QString(".");
		trayMessage = message;
		m_timer.start();
	}

	if (m_pomodoroGroupBox->isChecked())
	{
		QString message;
		int i = 0;
		QStringList list{ QString("Pomodoro set to"), QString("\nShort break set to"), QString("\nLong break set to")};
		for (QTime time: { m_pomodoroTime->time(), m_shortBreakTime->time(), m_longBreakTime->time() } )
		{
			message += list[i++];

			if (time.hour() != 0)
			{
				message += QString(" %1 hour").arg(time.hour());

				if (time.hour() > 1)
					message += QString("s");
			}

			if (time.minute() != 0)
			{
				message += QString(" %1 minute").arg(time.minute());

				if (time.minute() > 1)
					message += QString("s");
			}

			if (time.second() != 0)
			{
				message += QString(" %1 second").arg(time.second());

				if (time.second() > 1)
					message += QString("s");
			}

			message += QString(".");
		}

		if (m_captureGroupBox->isChecked())
			trayMessage += QString("\n");

		trayMessage += message;

		if (m_pomodoroAnimateTray->isChecked() && (m_trayIcon != nullptr))
			m_trayIcon->setIcon(QIcon(":/DesktopCapture/0-red.ico"));

		updatePomodoroValues();
		connect(&m_pomodoro, SIGNAL(progress(unsigned int)), this, SLOT(updateTrayProgress()), Qt::DirectConnection);
		connect(&m_pomodoro, SIGNAL(pomodoroEnded()), this, SLOT(trayMessage()), Qt::DirectConnection);
		connect(&m_pomodoro, SIGNAL(shortBreakEnded()), this, SLOT(trayMessage()), Qt::DirectConnection);
		connect(&m_pomodoro, SIGNAL(longBreakEnded()), this, SLOT(trayMessage()), Qt::DirectConnection);
		connect(&m_pomodoro, SIGNAL(sessionEnded()), this, SLOT(trayMessage()), Qt::DirectConnection);
		m_pomodoro.start();
	}

	if (m_trayIcon != nullptr)
	{
		QMenu *menu = m_trayIcon->contextMenu();
		if (m_pomodoroGroupBox->isChecked())
		{
			menu->addAction(m_menuShowStats);
			menu->addAction(m_menuPause);
			menu->addAction(m_menuChangeTask);
		}
		menu->addAction(m_menuStopCapture);
		menu->addSeparator();
		menu->addAction(m_menuQuit);

		if (m_pomodoroGroupBox->isChecked())
			m_trayIcon->setToolTip(m_pomodoro.statusMessage());
		else
			m_trayIcon->setToolTip(QString("Capturing desktop."));

		m_trayIcon->show();
		m_trayIcon->showMessage(QString("Started"), trayMessage, QSystemTrayIcon::MessageIcon::Information, 1000);
	}

	setWindowState(windowState() | Qt::WindowMinimized | Qt::WindowActive);
}

//-----------------------------------------------------------------
void DesktopCapture::capture()
{
	QIcon icon;
	if (m_screenshotAnimateTray->isChecked() && (m_trayIcon != nullptr))
	{
		icon = m_trayIcon->icon();
		m_trayIcon->setIcon(QIcon(":/DesktopCapture/application-shot.ico"));
	}

	if (m_captureThread)
	{
		m_captureThread->takeScreenshot();
		auto pixmap = m_captureThread->getImage();

		if(!m_captureVideo->isChecked())
		{
			saveCapture(pixmap);
		}
		else
		{
			if (m_secuentialNumber == 0)
			{
				QString fileName = this->m_dirEditLabel->text() + QString("\\DesktopCapture_") + QDateTime::currentDateTime().toString("dd_MM_yyyy") + QString(".webm");

				QRect desktopGeometry;
				if (this->m_captureAllMonitors->isChecked())
					desktopGeometry = QApplication::desktop()->geometry();
				else
					desktopGeometry = QApplication::desktop()->screenGeometry(this->m_captureMonitorComboBox->currentIndex());

				m_vp8_interface = new VPX_Interface(fileName, desktopGeometry.height(), desktopGeometry.width(), m_fps->value());
			}

			auto image = pixmap->toImage().convertToFormat(QImage::Format_RGB32);
			m_vp8_interface->encodeFrame(&image);
		}
	}

	++m_secuentialNumber;

	if (m_screenshotAnimateTray->isChecked() && (m_trayIcon != nullptr))
		m_trayIcon->setIcon(icon);
}

//-----------------------------------------------------------------
void DesktopCapture::updateCaptureDesktop(bool status)
{
	switch(status)
	{
		case true:
			if (m_captureThread && m_captureThread->isPaused())
				m_captureThread->resume();

			if (!m_captureThread)
			{
				setupCameraResolutions();
				setupCaptureThread();
			}
			m_startButton->setEnabled(true);
			break;
		case false:
			if (m_captureThread && !m_captureThread->isPaused())
				m_captureThread->pause();

			m_screenshotImage->clear();
			m_startButton->setEnabled(m_pomodoroGroupBox->isChecked());
			break;
		default:
			break;
	}

	m_screenshotImage->setEnabled(status);
	m_screeshotTime->setEnabled(status);
	m_dirButton->setEnabled(status);
	m_dirEditLabel->setEnabled(status);
	m_fps->setEnabled(status);
}

//-----------------------------------------------------------------
void DesktopCapture::updatePomodoro(bool status)
{
	m_startButton->setEnabled(m_captureGroupBox->isChecked() || status);
	m_continuousTicTac->setEnabled(status && m_pomodoroUseSounds->isChecked());
	m_overlayStats->setEnabled(status);
	if (m_captureThread)
		m_captureThread->setPomodoro((status && m_captureGroupBox->isChecked() && this->m_overlayStats->isChecked()) ? &m_pomodoro : nullptr);
}

//-----------------------------------------------------------------
void DesktopCapture::updateCameraPositionComboBox(int status)
{
	if (m_captureThread)
	{
		m_captureThread->setCameraOverlayPosition(m_cameraPositionComboBox->currentText());
		m_PIPposition = m_captureThread->getCameraOverlayPosition();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::updateTrayProgress()
{
	if (m_trayIcon != nullptr)
		m_trayIcon->setIcon(m_pomodoro.icon());
}

//-----------------------------------------------------------------
void DesktopCapture::trayMessage()
{
	QString message;
	QString tooltip;
	Pomodoro::Status nextStatus;

	if (m_pomodoro.status() == Pomodoro::Status::Stopped)
	{
		disconnect(&m_pomodoro, SIGNAL(pomodoroEnded()), this, SLOT(trayMessage()));
		disconnect(&m_pomodoro, SIGNAL(shortBreakEnded()), this, SLOT(trayMessage()));
		disconnect(&m_pomodoro, SIGNAL(longBreakEnded()), this, SLOT(trayMessage()));
		disconnect(&m_pomodoro, SIGNAL(sessionEnded()), this, SLOT(trayMessage()));
		message = QString("Session pomodoros completed!");
		nextStatus = Pomodoro::Status::Stopped;
	}

	if (m_pomodoro.status() == Pomodoro::Status::Pomodoro)
	{
		message = QString("Completed a pomodoro.\nStarting a short break.");
		tooltip = QString("In a short break.\nCompleted %1 pomodoros.").arg(m_pomodoro.completedPomodoros());
		nextStatus = Pomodoro::Status::ShortBreak;
	}

	if (m_pomodoro.status() == Pomodoro::Status::ShortBreak)
	{
		message = QString("Competed a short break.\nStarting a ");
		if ((m_pomodoro.completedPomodoros() % m_pomodorosBreakNumber->value()) == 0)
		{
			message += QString("long break.");
			tooltip = QString("In a long break.\nCompleted %1 pomodoros.").arg(m_pomodoro.completedPomodoros());
			nextStatus = Pomodoro::Status::LongBreak;
		}
		else
		{
			message += QString("pomodoro.");
			tooltip = QString("In a pomodoro.\nCompleted %1 pomodoros.").arg(m_pomodoro.completedPomodoros());
			nextStatus = Pomodoro::Status::Pomodoro;
		}
	}

	if (m_pomodoro.status() == Pomodoro::Status::LongBreak)
	{
		message = QString("Competed a long break.");
		tooltip = QString("Completed %1 pomodoros.").arg(m_pomodoro.completedPomodoros());
		if (m_pomodoro.completedPomodoros() != static_cast<unsigned int>(m_pomodorosNumber->value()))
		{
			message += QString("\nStarting a pomodoro.");
			tooltip = QString("In a pomodoro.\nCompleted %1 pomodoros.").arg(m_pomodoro.completedPomodoros());
			nextStatus = Pomodoro::Status::Pomodoro;
		}
		else
			nextStatus = Pomodoro::Status::Stopped;
	}

	if (m_trayIcon != nullptr)
	{
		m_trayIcon->showMessage(QString("Pomodoro Timer"), message, QSystemTrayIcon::MessageIcon::NoIcon, 1000);
		m_trayIcon->setToolTip(tooltip);
		updateTrayProgress();
	}

	if (m_statisticsDialog != nullptr)
		m_statisticsDialog->updateGUI(nextStatus);
}

//-----------------------------------------------------------------
void DesktopCapture::updateContinuousTicTac(int status)
{
	bool value = (status == Qt::Checked);
	m_pomodoro.setContinuousTicTac(value);
}

//-----------------------------------------------------------------
void DesktopCapture::updateUseSounds(int status)
{
	bool value = (status == Qt::Checked);
	m_pomodoro.setUseSounds(value);
	m_continuousTicTac->setEnabled(value);
}

//-----------------------------------------------------------------
void DesktopCapture::statisticsDialogClosed()
{
	disconnect(m_statisticsDialog, SIGNAL(finished(int)), this, SLOT(statisticsDialogClosed()));
	PomodoroStatistics::Result result = m_statisticsDialog->getResult();

	delete m_statisticsDialog;
	m_statisticsDialog = nullptr;

	if (result == PomodoroStatistics::Result::Stop)
		stopCaptureAction();
}

//-----------------------------------------------------------------
void DesktopCapture::updateTaskName()
{
	 changeTask();

	 this->m_taskEditButton->setDown(false);
}

//-----------------------------------------------------------------
void DesktopCapture::stopCaptureAction()
{
	stopCapture();

	show();
	setWindowState(windowState() & (~Qt::WindowMinimized | Qt::WindowActive));
}

//-----------------------------------------------------------------
void DesktopCapture::stopCapture()
{
	m_started = false;

	if (m_captureGroupBox->isChecked())
		disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(capture()));

	if (m_pomodoroGroupBox->isChecked())
	{
		disconnect(&m_pomodoro, SIGNAL(pomodoroEnded()), this, SLOT(trayMessage()));
		disconnect(&m_pomodoro, SIGNAL(shortBreakEnded()), this, SLOT(trayMessage()));
		disconnect(&m_pomodoro, SIGNAL(longBreakEnded()), this, SLOT(trayMessage()));
		disconnect(&m_pomodoro, SIGNAL(sessionEnded()), this, SLOT(trayMessage()));
		m_pomodoroTask->setText(m_pomodoro.getTask());
		m_pomodoro.stop();
		m_pomodoro.clear();
	}

	if (m_captureThread)
	{
		if(m_captureVideo->isChecked())
			delete m_vp8_interface;

		m_secuentialNumber = 0;
		m_captureThread->resume();
	}

	m_trayIcon->hide();
	m_trayIcon->setIcon(QIcon(":/DesktopCapture/application.ico"));

	QMenu *menu = m_trayIcon->contextMenu();
	for(auto action: menu->actions())
	{
		menu->removeAction(action);

		if(action->isSeparator())
			delete action;
	}
}

//-----------------------------------------------------------------
void DesktopCapture::showStatistics()
{
	if (m_statisticsDialog == nullptr)
	{
		m_statisticsDialog = new PomodoroStatistics(&m_pomodoro, m_paused, this);
		connect(m_statisticsDialog, SIGNAL(finished(int)), this, SLOT(statisticsDialogClosed()), Qt::QueuedConnection);
		m_statisticsDialog->show();
	}

	m_statisticsDialog->raise();
}

//-----------------------------------------------------------------
void DesktopCapture::changeTask()
{
	bool ok;
	QString text = QInputDialog::getText(this,
                                       tr("Enter task name"),
				                               tr("Task name:"),
				                               QLineEdit::Normal,
				                               m_pomodoroTask->text(), &ok);
	if (ok && !text.isEmpty())
	{
		m_pomodoroTask->setText(text);
		m_pomodoro.setTask(m_pomodoroTask->text());
	}
}

//-----------------------------------------------------------------
void DesktopCapture::pausePomodoro()
{
	m_paused = true;
	m_pomodoro.pause();
	showStatistics();
}

//-----------------------------------------------------------------
void DesktopCapture::quitApplication()
{
	m_pomodoro.stop();
	stopCapture();
	QApplication::instance()->quit();
}

//-----------------------------------------------------------------
void DesktopCapture::updatePomodoroOverlay(int status)
{
	if(m_captureThread)
		m_captureThread->setPomodoro(status ? &m_pomodoro : nullptr);
}

//-----------------------------------------------------------------
void DesktopCapture::updatePomodoroValues()
{
	m_pomodoro.setPomodoroTime(m_pomodoroTime->time());
	m_pomodoro.setShortBreakTime(m_shortBreakTime->time());
	m_pomodoro.setLongBreakTime(m_longBreakTime->time());
	m_pomodoro.setPomodorosBeforeBreak(m_pomodorosBreakNumber->value());
	m_pomodoro.setSessionPodomodos(m_pomodorosNumber->value());
	m_pomodoro.setTask(m_pomodoroTask->text());
}
