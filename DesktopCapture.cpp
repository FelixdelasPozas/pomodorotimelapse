/*
    File: DesktopCapture.cpp
    Created on: 21/06/2013
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
#include "DesktopCapture.h"
#include "ProbeResolutionsDialog.h"
#include "CaptureDesktopThread.h"
#include "PomodoroStatistics.h"
#include "AboutDialog.h"
#include "Pomodoro.h"
#include "VPXInterface.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// Qt
#include <QtGui>
#include <QString>
#include <QPixmap>
#include <QDebug>
#include <QSettings>
#include <QDesktopWidget>
#include <QInputDialog>
#include <QFileDialog>

const QStringList DesktopCapture::COMPOSITION_MODES_NAMES = { "Copy",
                                                              "Plus",
                                                              "Multiply" };

const QStringList DesktopCapture::POSITION_NAMES = { "Free",
                                                     "Top Left",
                                                     "Top Center",
                                                     "Top Right",
                                                     "Center Left",
                                                     "Center",
                                                     "Center Right",
                                                     "Bottom Left",
                                                     "Bottom Center",
                                                     "Bottom Right" };

const QString DesktopCapture::CAPTURE_TIME                       = "Time Between Captures";
const QString DesktopCapture::CAPTURE_ENABLED                    = "Enable Desktop Capture";
const QString DesktopCapture::CAPTURE_VIDEO                      = "Capture Video";
const QString DesktopCapture::CAPTURE_VIDEO_FPS                  = "Capture Video FPS";
const QString DesktopCapture::CAPTURE_ANIMATED_TRAY_ENABLED      = "Capture Animated Tray Icon";
const QString DesktopCapture::CAPTURED_MONITOR                   = "Captured Desktop Monitor";
const QString DesktopCapture::MONITORS_LIST                      = "Monitor Resolutions";
const QString DesktopCapture::OUTPUT_DIR                         = "Output Directory";
const QString DesktopCapture::OUTPUT_SCALE                       = "Output Scale";
const QString DesktopCapture::APPLICATION_GEOMETRY               = "Application Geometry";
const QString DesktopCapture::APPLICATION_STATE                  = "Application State";
const QString DesktopCapture::CAMERA_ENABLED                     = "Camera Enabled";
const QString DesktopCapture::CAMERA_RESOLUTIONS                 = "Available Camera Resolutions";
const QString DesktopCapture::CAMERA_ACTIVE_RESOLUTION           = "Active Resolution";
const QString DesktopCapture::CAMERA_OVERLAY_POSITION            = "Camera Overlay Position";
const QString DesktopCapture::CAMERA_OVERLAY_COMPOSITION_MODE    = "Camera Overlay Composition Mode";
const QString DesktopCapture::CAMERA_OVERLAY_FIXED_POSITION      = "Camera Overlay Fixed Position";
const QString DesktopCapture::CAMERA_MASK                        = "Camera Mask";
const QString DesktopCapture::CAMERA_TRACK_FACE                  = "Center face in camera picture";
const QString DesktopCapture::CAMERA_ASCII_ART                   = "Convert camera picture to ASCII art";
const QString DesktopCapture::POMODORO_TIME                      = "Pomodoro Time";
const QString DesktopCapture::POMODORO_SHORT_BREAK_TIME          = "Short Break Time";
const QString DesktopCapture::POMODORO_LONG_BREAK_TIME           = "Long Break Time";
const QString DesktopCapture::POMODOROS_BEFORE_BREAK             = "Pomodoros Before A Long Break";
const QString DesktopCapture::POMODOROS_ANIMATED_TRAY_ENABLED    = "Pomodoro Animated Tray Icon";
const QString DesktopCapture::POMODOROS_USE_SOUNDS               = "Pomodoro Use Sounds";
const QString DesktopCapture::POMODORO_ENABLED                   = "Enable Pomodoro";
const QString DesktopCapture::POMODOROS_CONTINUOUS_TICTAC        = "Continuous Tic-Tac";
const QString DesktopCapture::POMODOROS_SESSION_NUMBER           = "Pomodoros In Session";
const QString DesktopCapture::POMODOROS_LAST_TASK                = "Last task";
const QString DesktopCapture::POMODOROS_OVERLAY                  = "Overlay Pomodoro Statistics In Capture";
const QString DesktopCapture::POMODOROS_OVERLAY_POSITION         = "Pomodoro Overlay Position";
const QString DesktopCapture::POMODOROS_OVERLAY_FIXED_POSITION   = "Pomodoro Overlay Fixed Position";
const QString DesktopCapture::POMODOROS_OVERLAY_COMPOSITION_MODE = "Pomodoro Overlay Composition Mode";

//-----------------------------------------------------------------
DesktopCapture::DesktopCapture()
: m_trayIcon        {nullptr}
, m_captureThread   {nullptr}
, m_secuentialNumber{0}
, m_started         {false}
, m_statisticsDialog{nullptr}
, m_paused          {false}
, m_menuPause       {nullptr}
, m_menuShowStats   {nullptr}
, m_menuStopCapture {nullptr}
, m_menuChangeTask  {nullptr}
, m_menuQuit        {nullptr}
{
	setupUi(this);

	setWindowTitle("Desktop Capture");
	setWindowIcon(QIcon(":/DesktopCapture/application.ico"));

	m_pomodoro = std::make_shared<Pomodoro>();

	loadConfiguration();

	setupTrayIcon();
	setupCameraResolutions();

	if (m_captureGroupBox->isChecked())
	{
	  setupCaptureThread();
	}

	connectSignals();

	m_screenshotImage->installEventFilter(this);

	if (m_screenshotImage->underMouse() && m_captureThread)
	{
		m_captureThread->setPaintFrame(true);
	}
}

//-----------------------------------------------------------------
DesktopCapture::~DesktopCapture()
{
	if (m_captureThread)
	{
		if(m_started)
		{
			m_vp8_interface = nullptr;
		}

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
  QSettings settings("Felix de las Pozas Alvarez", "DesktopCapture");

	if (settings.contains(APPLICATION_GEOMETRY))
	{
		restoreGeometry(settings.value(APPLICATION_GEOMETRY).toByteArray());
	}
	else
	{
		showMaximized();
	}

	if (settings.contains(APPLICATION_STATE))
	{
		restoreState(settings.value(APPLICATION_STATE).toByteArray());
	}

	QStringList monitors;
	if (settings.contains(MONITORS_LIST))
	{
		monitors = settings.value(MONITORS_LIST, QStringList()).toStringList();
	}
	else
	{
		QDesktopWidget *desktop = QApplication::desktop();
		for (int i = 0; i < desktop->numScreens(); ++i)
		{
			auto geometry = desktop->screenGeometry(i);
			if (desktop->primaryScreen() == i)
			{
				monitors << QString("Primary Screen (Size: %1x%2 - Position: %3x%4)").arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
			}
			else
			{
				monitors << QString("Additional Screen %1 (Size: %2x%3 - Position: %4x%5)").arg(i).arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
			}
		}
	}
	m_captureMonitorComboBox->insertItems(0, monitors);

  int capturedMonitor = -1;
  if (settings.contains(CAPTURED_MONITOR))
  {
    capturedMonitor = settings.value(CAPTURED_MONITOR, capturedMonitor).toInt();
  }
  m_captureAllMonitors->setChecked(capturedMonitor == -1);
  m_captureMonitorComboBox->setCurrentIndex((capturedMonitor != -1) ? capturedMonitor : 0);
  m_captureMonitorComboBox->setEnabled(!this->m_captureAllMonitors->isChecked());

  QPoint position(0,0);
	if (settings.contains(CAMERA_OVERLAY_POSITION))
	{
		position = settings.value(CAMERA_OVERLAY_POSITION, position).toPoint();
	}
	m_PIPposition = position;

	bool pomodoroEnabled = true;
	if (settings.contains(POMODORO_ENABLED))
	{
		pomodoroEnabled = settings.value(POMODORO_ENABLED, pomodoroEnabled).toBool();
	}
	m_pomodoroGroupBox->setChecked(pomodoroEnabled);

	auto pomodoroTime = QTime(0,25,0);
	if (settings.contains(POMODORO_TIME))
	{
		pomodoroTime = settings.value(POMODORO_TIME, pomodoroTime).toTime();
	}
	m_pomodoroTime->setTime(pomodoroTime);
	m_pomodoro->setPomodoroDuration(pomodoroTime);

	auto shortBreak = QTime(0,5,0);
	if (settings.contains(POMODORO_SHORT_BREAK_TIME))
	{
		shortBreak = settings.value(POMODORO_SHORT_BREAK_TIME, shortBreak).toTime();
	}
	m_shortBreakTime->setTime(shortBreak);
	m_pomodoro->setShortBreakDuration(shortBreak);

	QTime longBreak = QTime(0,15,0);
	if (settings.contains(POMODORO_LONG_BREAK_TIME))
	{
		longBreak = settings.value(POMODORO_LONG_BREAK_TIME, longBreak).toTime();
	}
	m_longBreakTime->setTime(longBreak);
	m_pomodoro->setLongBreakDuration(longBreak);

	int pomodorosBeforeBreak = 4;
	if (settings.contains(POMODOROS_BEFORE_BREAK))
	{
		pomodorosBeforeBreak = settings.value(POMODOROS_BEFORE_BREAK, pomodorosBeforeBreak).toInt();
	}
	m_pomodorosBreakNumber->setValue(pomodorosBeforeBreak);
	m_pomodoro->setPomodorosBeforeBreak(pomodorosBeforeBreak);

	auto animatePomodoro = true;
	if (settings.contains(POMODOROS_ANIMATED_TRAY_ENABLED))
	{
		animatePomodoro = settings.value(POMODOROS_ANIMATED_TRAY_ENABLED, animatePomodoro).toBool();
	}
	m_pomodoroAnimateTray->setChecked(animatePomodoro);

	auto pomodoroUseSounds = true;
	if (settings.contains(POMODOROS_USE_SOUNDS))
	{
		pomodoroUseSounds = settings.value(POMODOROS_USE_SOUNDS, pomodoroUseSounds).toBool();
	}
	m_pomodoroUseSounds->setChecked(pomodoroUseSounds);
	m_pomodoro->setUseSounds(pomodoroUseSounds);

	auto pomodoroContinuousTicTac = false;
	if (settings.contains(POMODOROS_CONTINUOUS_TICTAC))
	{
		pomodoroContinuousTicTac = settings.value(POMODOROS_CONTINUOUS_TICTAC, pomodoroContinuousTicTac).toBool();
	}
	m_continuousTicTac->setChecked(pomodoroContinuousTicTac);
	m_continuousTicTac->setEnabled(pomodoroUseSounds && this->m_pomodoroGroupBox->isChecked());
	m_pomodoro->setContinuousTicTac(pomodoroContinuousTicTac);

	unsigned int pomodorosInSession = 12;
	if (settings.contains(POMODOROS_SESSION_NUMBER))
	{
		pomodorosInSession = settings.value(POMODOROS_SESSION_NUMBER, pomodorosInSession).toUInt();
	}
	m_pomodorosNumber->setValue(pomodorosInSession);
	m_pomodoro->setSessionPodomodos(pomodorosInSession);

	auto task = QString("Undefined task");
	if (settings.contains(POMODOROS_LAST_TASK))
	{
		task = settings.value(POMODOROS_LAST_TASK, task).toString();
	}
	m_pomodoroTask->setText(task);
	m_pomodoro->setTask(task);

	auto overlayPomodoro = true;
	if (settings.contains(POMODOROS_OVERLAY))
	{
		overlayPomodoro = settings.value(POMODOROS_OVERLAY, overlayPomodoro).toBool();
	}
	m_overlayStats->setChecked(overlayPomodoro);
	m_overlayStats->setEnabled(m_pomodoroGroupBox->isChecked());

  m_statsPosition = QPoint(0,0);
	if (settings.contains(POMODOROS_OVERLAY_POSITION))
	{
		m_statsPosition = settings.value(POMODOROS_OVERLAY_POSITION, m_statsPosition).toPoint();
	}

  int statsPosition = 0;
  if (settings.contains(POMODOROS_OVERLAY_FIXED_POSITION))
  {
    statsPosition = settings.value(POMODOROS_OVERLAY_FIXED_POSITION, statsPosition).toInt();
  }
  m_pomodoroPositionComboBox->insertItems(0, POSITION_NAMES);
  m_pomodoroPositionComboBox->setCurrentIndex(statsPosition);
  m_pomodoroPositionComboBox->setEnabled(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked());
  m_pomodoroPositionLabel->setEnabled(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked());

  int modeIndex = 0;
  if (settings.contains(POMODOROS_OVERLAY_COMPOSITION_MODE))
  {
    modeIndex = settings.value(POMODOROS_OVERLAY_COMPOSITION_MODE, modeIndex).toInt();
  }
  m_pomodoroCompositionComboBox->insertItems(0, COMPOSITION_MODES_NAMES);
  m_pomodoroCompositionComboBox->setCurrentIndex(modeIndex);
  m_pomodoroCompositionComboBox->setEnabled(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked());
  m_pomodoroCompositionLabel->setEnabled(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked());

	auto timeBetweenCaptures = QTime(0,0,30);
	if (settings.contains(CAPTURE_TIME))
	{
		timeBetweenCaptures = settings.value(CAPTURE_TIME, timeBetweenCaptures).toTime();
	}
	m_screenshotTime->setTime(timeBetweenCaptures);

	auto captureVideo = true;
	if (settings.contains(CAPTURE_VIDEO))
	{
		captureVideo = settings.value(CAPTURE_VIDEO, captureVideo).toBool();
	}
	m_videoRadioButton->setChecked(captureVideo);
	m_screenshotsRadioButton->setChecked(!captureVideo);

	int videoFps = 15;
	if (settings.contains(CAPTURE_VIDEO_FPS))
	{
		videoFps = settings.value(CAPTURE_VIDEO_FPS, videoFps).toInt();
	}
	m_fps->setEnabled(captureVideo);
	m_fps->setValue(videoFps);

	auto outputDir = QDir::homePath();
	if (settings.contains(OUTPUT_DIR))
	{
		outputDir = settings.value(OUTPUT_DIR, outputDir).toString();
	}
	m_dirEditLabel->setText(outputDir.replace('/', QDir::separator()));

	int scale = 1;
	if(settings.contains(OUTPUT_SCALE))
	{
	  scale = settings.value(OUTPUT_SCALE, scale).toInt();
	}
	onScaleIndexChanged(scale);
  m_scaleComboBox->setCurrentIndex(scale);

	auto captureEnabled = true;
	if (settings.contains(CAPTURE_ENABLED))
	{
		captureEnabled = settings.value(CAPTURE_ENABLED, captureEnabled).toBool();
	}
	m_captureGroupBox->setChecked(captureEnabled);

  bool enableCamera = true;
  if (settings.contains(CAMERA_ENABLED))
  {
    enableCamera = settings.value(CAMERA_ENABLED, enableCamera).toBool();
  }
  m_cameraEnabled->setChecked(enableCamera);

  modeIndex = 0;
  if (settings.contains(CAMERA_OVERLAY_COMPOSITION_MODE))
  {
    modeIndex = settings.value(CAMERA_OVERLAY_COMPOSITION_MODE, modeIndex).toInt();
  }
  m_compositionComboBox->insertItems(0, COMPOSITION_MODES_NAMES);
  m_compositionComboBox->setCurrentIndex(modeIndex);
  m_compositionComboBox->setEnabled(enableCamera);

  int cameraPosition = 0;
  if (settings.contains(CAMERA_OVERLAY_FIXED_POSITION))
  {
    cameraPosition = settings.value(CAMERA_OVERLAY_FIXED_POSITION, cameraPosition).toInt();
  }
  m_cameraPositionComboBox->insertItems(0, POSITION_NAMES);
  m_cameraPositionComboBox->setCurrentIndex(cameraPosition);
  m_cameraPositionComboBox->setEnabled(enableCamera);

  QStringList masks;
  for(auto mask: CaptureDesktopThread::MASKS)
  {
    masks << mask.name;
  }
  masks << QString("None");
  m_cameraMaskComboBox->insertItems(0, masks);
  int cameraMaskIndex = m_cameraMaskComboBox->count()-1;
  if(settings.contains(CAMERA_MASK))
  {
    cameraMaskIndex = settings.value(CAMERA_MASK, cameraMaskIndex).toInt();
  }
  m_cameraMaskComboBox->setCurrentIndex(cameraMaskIndex);

  auto trackFace = false;
  if(settings.contains(CAMERA_TRACK_FACE))
  {
    trackFace = settings.value(CAMERA_TRACK_FACE, trackFace).toBool();
  }
  m_trackFace->setChecked(trackFace);

  auto convertToASCII = false;
  if(settings.contains(CAMERA_ASCII_ART))
  {
    trackFace = settings.value(CAMERA_ASCII_ART, convertToASCII).toBool();
  }
  m_ASCIIart->setChecked(convertToASCII);

  bool animateScreenshot = true;
  if (settings.contains(CAPTURE_ANIMATED_TRAY_ENABLED))
  {
    animateScreenshot = settings.value(CAPTURE_ANIMATED_TRAY_ENABLED, animateScreenshot).toBool();
  }
	m_screenshotAnimateTray->setChecked(animateScreenshot);

  m_selectedResolution = 0;
  if (settings.contains(CAMERA_ACTIVE_RESOLUTION))
  {
    m_selectedResolution = settings.value(CAMERA_ACTIVE_RESOLUTION, m_selectedResolution).toInt();
  }

  if (settings.contains(CAMERA_RESOLUTIONS))
  {
    m_cameraResolutionsNames = settings.value(CAMERA_RESOLUTIONS, QStringList()).toStringList();
  }

	computeVideoTime();
}

//-----------------------------------------------------------------
void DesktopCapture::saveConfiguration()
{
  QSettings settings("Felix de las Pozas Alvarez", "DesktopCapture");
	int capturedMonitor = (m_captureAllMonitors->isChecked() ? -1 : m_captureMonitorComboBox->currentIndex());

	settings.setValue(CAPTURED_MONITOR, capturedMonitor);

	QStringList monitors;
	for (int i = 0; i < m_captureMonitorComboBox->count(); ++i)
	{
		monitors << m_captureMonitorComboBox->itemText(i);
	}

	settings.setValue(MONITORS_LIST, monitors);
	settings.setValue(OUTPUT_DIR, m_dirEditLabel->text());
	settings.setValue(OUTPUT_SCALE, m_scaleComboBox->currentIndex());
	settings.setValue(CAMERA_ENABLED, m_cameraEnabled->isChecked());
	settings.setValue(CAPTURE_ANIMATED_TRAY_ENABLED, m_screenshotAnimateTray->isChecked());
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
  settings.setValue(CAPTURE_TIME, m_screenshotTime->time());
  settings.setValue(CAPTURE_ENABLED, m_captureGroupBox->isChecked());
  settings.setValue(CAPTURE_VIDEO, m_videoRadioButton->isChecked());
  settings.setValue(CAPTURE_VIDEO_FPS, m_fps->value());
  settings.setValue(CAMERA_OVERLAY_FIXED_POSITION, m_cameraPositionComboBox->currentIndex());
  settings.setValue(CAMERA_MASK, m_cameraMaskComboBox->currentIndex());
  settings.setValue(CAMERA_TRACK_FACE, m_trackFace->isChecked());
  settings.setValue(POMODOROS_LAST_TASK, m_pomodoroTask->text());
  settings.setValue(POMODOROS_OVERLAY, m_overlayStats->isChecked());
  settings.setValue(POMODOROS_OVERLAY_POSITION, m_statsPosition);
  settings.setValue(POMODOROS_OVERLAY_FIXED_POSITION, m_pomodoroPositionComboBox->currentIndex());
  settings.setValue(CAMERA_ASCII_ART, m_ASCIIart->isChecked());
  settings.setValue(POMODOROS_OVERLAY_COMPOSITION_MODE, m_pomodoroCompositionComboBox->currentIndex());

	settings.sync();
}

//-----------------------------------------------------------------
void DesktopCapture::connectSignals() const
{
  connect(m_cameraEnabled, SIGNAL(clicked(bool)),
          this,            SLOT(onCameraStateChanged(bool)));

  connect(m_dirButton, SIGNAL(pressed()),
          this,        SLOT(onDirButtonPressed()));

  connect(m_cameraResolutionComboBox, SIGNAL(currentIndexChanged(int)),
          this,                       SLOT(onCameraResolutionChanged(int)));

  connect(m_compositionComboBox, SIGNAL(currentIndexChanged(int)),
          this,                  SLOT(onCameraCompositionModeChanged(int)));

  connect(m_startButton, SIGNAL(pressed()),
          this,          SLOT(onStartButtonPressed()));

  connect(m_captureGroupBox, SIGNAL(clicked(bool)),
          this,              SLOT(onCaptureGroupChanged(bool)));

  connect(m_pomodoroGroupBox, SIGNAL(clicked(bool)),
          this,               SLOT(onPomodoroGroupChanged(bool)));

  connect(m_cameraPositionComboBox, SIGNAL(currentIndexChanged(int)),
          this,                     SLOT(onCameraPositionChanged(int)));

  connect(m_pomodoroPositionComboBox, SIGNAL(currentIndexChanged(int)),
          this,                       SLOT(onPomodoroPositionChanged(int)));

  connect(m_continuousTicTac, SIGNAL(stateChanged(int)),
          this,               SLOT(onTicTacStateChanged(int)));

  connect(m_pomodoroUseSounds, SIGNAL(stateChanged(int)),
          this,                SLOT(onPomodoroSoundsChanged(int)));

  connect(m_captureAllMonitors, SIGNAL(stateChanged(int)),
          this,                 SLOT(onMonitorsStateChanged(int)));

  connect(m_captureMonitorComboBox, SIGNAL(currentIndexChanged(int)),
          this,                     SLOT(onMonitorValueChanged(int)));

  connect(m_taskEditButton, SIGNAL(pressed()),
          this,             SLOT(onTaskButtonPressed()));

  connect(m_overlayStats, SIGNAL(stateChanged(int)),
          this,           SLOT(onStatisticsStateChanged(int)));

  connect(m_pomodorosBreakNumber, SIGNAL(valueChanged(int)),
          this,                   SLOT(onPomodoroValuesChanged()));

  connect(m_pomodorosNumber, SIGNAL(valueChanged(int)),
          this,              SLOT(onPomodoroValuesChanged()));

  connect(m_videoRadioButton, SIGNAL(toggled(bool)),
          this,               SLOT(onCaptureVideoChanged(bool)));

  connect(m_screenshotsRadioButton, SIGNAL(toggled(bool)),
          this,                     SLOT(onCaptureVideoChanged(bool)));

  connect(m_scaleComboBox, SIGNAL(currentIndexChanged(int)),
          this,            SLOT(onScaleIndexChanged(int)));

  connect(m_cameraMaskComboBox, SIGNAL(currentIndexChanged(int)),
          this,                 SLOT(onMaskIndexChanged(int)));

  connect(m_pomodoroCompositionComboBox, SIGNAL(currentIndexChanged(int)),
          this,                          SLOT(onPomodoroCompositionModeChanged(int)));

  connect(m_trackFace, SIGNAL(stateChanged(int)),
          this,        SLOT(onFaceTrackingChanged(int)));

  connect(m_ASCIIart, SIGNAL(stateChanged(int)),
          this,       SLOT(onConvertToASCIIChanged(int)));

  connect(m_about, SIGNAL(pressed()),
          this,    SLOT(onAboutButtonPressed()));

  connect(m_quit, SIGNAL(clicked()), 
	        this,   SLOT(quitApplication()));					

	connect(m_fps, SIGNAL(valueChanged(int)), this, SLOT(computeVideoTime()));
	connect(m_screenshotTime, SIGNAL(timeChanged(QTime)), this, SLOT(computeVideoTime()));
}

//-----------------------------------------------------------------
void DesktopCapture::setupCameraResolutions()
{
	if (m_captureThread)
	{
		m_captureThread->pause();
	}

	cv::VideoCapture camera;
	if (!camera.open(0))
	{
		m_cameraEnabled->blockSignals(true);
		m_cameraEnabled->setChecked(false);
		m_cameraEnabled->blockSignals(false);
		m_cameraResolutions.clear();
		m_cameraResolutionComboBox->insertItem(0, QString("No cameras detected."));
		m_cameraResolutionComboBox->setEnabled(false);
		m_cameraEnabled->setEnabled(false);
	}
	else
	{
		camera.release();

		if (!m_cameraResolutionsNames.empty())
		{
			m_cameraResolutionComboBox->insertItems(0, m_cameraResolutionsNames);

			for(auto resolutionString: m_cameraResolutionsNames)
			{
				QStringList parts = resolutionString.split(" ");
				QStringList numbers = parts[0].split("x");

				bool correct;
				int width = numbers[0].toInt(&correct, 10);

				if (!correct)	continue;

				int height = numbers[1].toInt(&correct, 10);

				if (!correct)	continue;

				m_cameraResolutions << getResolution(width, height);
			}
		}
		else
		{
			ProbeResolutionsDialog dialog(this);
			dialog.setWindowIcon(QIcon(":/DesktopCapture/config.ico"));
			dialog.setWindowTitle(QString("Probing camera resolutions..."));
			dialog.exec();

			if (dialog.result() == QDialog::Accepted)
			{
				m_cameraResolutions = dialog.getResolutions();

				for (auto resolution: m_cameraResolutions)
				{
					m_cameraResolutionsNames << getResolutionAsString(resolution);
				}

				m_cameraResolutionComboBox->insertItems(0, m_cameraResolutionsNames);
				m_cameraResolutionComboBox->setCurrentIndex(m_cameraResolutionsNames.size() / 2);

				if (m_captureThread)
				{
					m_captureThread->setResolution(m_cameraResolutions.at(m_selectedResolution));
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
				{
					m_captureThread->setCameraEnabled(false);
				}
			}
		}
	}

	if (m_selectedResolution <= m_cameraResolutionComboBox->count())
	{
		m_cameraResolutionComboBox->setCurrentIndex(m_selectedResolution);
	}

	m_cameraResolutionComboBox->setEnabled(m_cameraEnabled->isChecked());

	if (m_captureThread && m_captureGroupBox->isChecked())
	{
		m_captureThread->resume();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onMonitorsStateChanged(int status)
{
	bool checked = (status == Qt::Checked);
	m_captureMonitorComboBox->setEnabled(!checked);

	if (m_captureThread)
	{
		if (checked)
		{
			m_captureThread->setMonitor(-1);
		}
		else
		{
			m_captureThread->setMonitor(m_captureMonitorComboBox->currentIndex());
		}

    recomputeOverlaysPositions();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onMonitorValueChanged(int index)
{
	if (m_captureThread)
	{
		m_captureThread->setMonitor(index);

		recomputeOverlaysPositions();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::recomputeOverlaysPositions()
{
  if (m_cameraEnabled)
  {
    if(m_cameraPositionComboBox->currentIndex() != 0)
    {
      auto position = static_cast<CaptureDesktopThread::POSITION>(m_cameraPositionComboBox->currentIndex());
      m_captureThread->setCameraOverlayPosition(position);
      m_PIPposition = m_captureThread->getCameraOverlayPosition();
    }
    else
    {
      computeNewPIPPosition();
      m_captureThread->setCameraOverlayPosition(m_PIPposition);
    }

    if(m_pomodoroPositionComboBox->currentIndex() != 0)
    {
      auto position = static_cast<CaptureDesktopThread::POSITION>(m_pomodoroPositionComboBox->currentIndex());
      m_captureThread->setStatsOverlayPosition(position);
      m_statsPosition = m_captureThread->getStatsOverlayPosition();
    }
    else
    {
      computeNewStatsPosition();
      m_captureThread->setStatsOverlayPosition(m_statsPosition);
    }
  }
}

//-----------------------------------------------------------------
void DesktopCapture::computeVideoTime() const
{
	const auto timeVal = m_screenshotTime->time();
	const auto totalTime = timeVal.second() + timeVal.minute()*60;
	const float frameNum = 3600.f/totalTime;
	const float fps = m_fps->value();
	m_computedTime->setText(tr("%1 hours per second. One real-time hour in %2 seconds. %3 frames/hour.").arg(fps/frameNum).arg(frameNum/fps).arg(frameNum));
}

//-----------------------------------------------------------------
void DesktopCapture::onCameraStateChanged(bool enabled)
{
	m_cameraResolutionComboBox->setEnabled(enabled);
	m_cameraPositionComboBox  ->setEnabled(enabled);
	m_compositionComboBox     ->setEnabled(enabled);

	if (m_cameraResolutions.isEmpty() && enabled)
	{
		setupCameraResolutions();
	}

	if (m_captureThread)
	{
		m_captureThread->setCameraEnabled(enabled);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::saveCapture(QPixmap *capture) const
{
	QString format("png");
	QString fileName = m_dirEditLabel->text() + tr("/DesktopCapture_") + QString("%1").arg(m_secuentialNumber,4,'d',0,'0') + QString(".") + format;

	if(m_scale != 1.0)
	{
	  capture->scaled(capture->size() * m_scale, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation).save(fileName, format.toStdString().c_str(), 0);
	}
	else
	{
	  capture->save(fileName, format.toStdString().c_str(), 0);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::setupTrayIcon()
{
	if(!QSystemTrayIcon::isSystemTrayAvailable()) return;

	QMenu *menu = new QMenu(this);
	m_trayIcon = new QSystemTrayIcon(this);
	m_trayIcon->setIcon(QIcon(":/DesktopCapture/application.ico"));
	m_trayIcon->setContextMenu(menu);

	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	        this,       SLOT(activateTrayIcon(QSystemTrayIcon::ActivationReason)));

	m_menuPause = new QAction(QString("Pause pomodoro"), menu);
	connect(m_menuPause, SIGNAL(triggered()),
	        this,        SLOT(pausePomodoro()), Qt::QueuedConnection);

  m_menuShow = new QAction(QString("Restore"), menu);
  connect(m_menuShow, SIGNAL(triggered()),
          this,   SLOT(showAction()), Qt::QueuedConnection);

	m_menuStopCapture = new QAction(QString("Stop"), menu);
	connect(m_menuStopCapture, SIGNAL(triggered()),
	        this,              SLOT(stopCaptureAction()), Qt::QueuedConnection);

	m_menuShowStats = new QAction(QString("Show statistics..."), menu);
	connect(m_menuShowStats, SIGNAL(triggered()),
	        this,            SLOT(showStatistics()), Qt::QueuedConnection);

	m_menuChangeTask = new QAction(QString("Change task..."), menu);
	connect(m_menuChangeTask, SIGNAL(triggered()),
	        this,             SLOT(changeTask()), Qt::QueuedConnection);

	m_menuAbout = new QAction(QString("About..."), menu);
  connect(m_menuAbout, SIGNAL(triggered()),
          this,        SLOT(onAboutButtonPressed()), Qt::QueuedConnection);

	m_menuQuit = new QAction(QString("Exit application"), menu);
	connect(m_menuQuit, SIGNAL(triggered()),
	        this,       SLOT(quitApplication()), Qt::QueuedConnection);
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

				if(!m_started)
				{
          QMenu *menu = m_trayIcon->contextMenu();
          menu->addAction(m_menuShow);
          menu->addSeparator();
          menu->addAction(m_menuAbout);
          menu->addSeparator();
          menu->addAction(m_menuQuit);

          m_trayIcon->show();
				}

        e->ignore();

        if (m_captureThread != nullptr)
        {
          m_captureThread->pause();
        }
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
	if ((reason) && (reason != QSystemTrayIcon::DoubleClick))	return;

	if(!m_started)
	{
		m_trayIcon->hide();

		clearTrayMenu();

		showAction();
	}
	else
	{
		if(m_pomodoroGroupBox->isChecked() && (m_pomodoro->status() != Pomodoro::Status::Stopped))
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
	{
		monitor = m_captureMonitorComboBox->currentIndex();
	}

	Resolution resolution{QString(), 0, 0};

	if (m_cameraEnabled->isChecked() && !m_cameraResolutions.empty())
	{
		resolution = m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex());
	}

	auto mask = static_cast<CaptureDesktopThread::MASK>(m_cameraMaskComboBox->currentIndex());
	auto cameraCompositionMode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_compositionComboBox->currentIndex());

	m_captureThread = new CaptureDesktopThread(monitor, resolution, this);
	m_captureThread->setCameraOverlayCompositionMode(cameraCompositionMode);
  m_captureThread->setTrackFace(m_trackFace->isChecked());
	m_captureThread->setMask(mask);

	if(m_cameraPositionComboBox->currentIndex() != 0)
	{
	  auto position = static_cast<CaptureDesktopThread::POSITION>(m_cameraPositionComboBox->currentIndex());
	  m_captureThread->setCameraOverlayPosition(position);
	  m_PIPposition = m_captureThread->getCameraOverlayPosition();
	}
	else
	{
	  m_captureThread->setCameraOverlayPosition(m_PIPposition);
	}

	if (m_pomodoroGroupBox->isChecked())
	{
    auto statisticsCompositonMode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_pomodoroCompositionComboBox->currentIndex());

		m_captureThread->setPomodoro(m_pomodoro);
	  m_captureThread->setStatisticsOverlayCompositionMode(statisticsCompositonMode);

	  if(m_pomodoroPositionComboBox->currentIndex() != 0)
	  {
	    auto position = static_cast<CaptureDesktopThread::POSITION>(m_pomodoroPositionComboBox->currentIndex());
	    m_captureThread->setStatsOverlayPosition(position);
	    m_statsPosition = m_captureThread->getStatsOverlayPosition();
	  }
	  else
	  {
	    m_captureThread->setStatsOverlayPosition(m_statsPosition);
	  }
	}

	connect(m_captureThread, SIGNAL(imageAvailable()),
	        this,            SLOT(renderImage()), Qt::QueuedConnection);

	m_captureThread->start(QThread::Priority::NormalPriority);
}

//-----------------------------------------------------------------
void DesktopCapture::renderImage()
{
	QMutexLocker lock(m_captureThread->getMutex());

	auto pixmap = m_captureThread->getImage();
	m_screenshotImage->setPixmap(pixmap->scaled(m_screenshotImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

//-----------------------------------------------------------------
void DesktopCapture::onDirButtonPressed()
{
	QDir dir(m_dirEditLabel->text());
	if (!dir.exists())
	{
		m_dirEditLabel->setText(QDir::currentPath());
	}

	QString dirText = QFileDialog::getExistingDirectory(this, tr("Open Directory"), m_dirEditLabel->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if(!dirText.isEmpty())
	{
	  m_dirEditLabel->setText(dirText.replace('/', QDir::separator()));
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCameraResolutionChanged(int index)
{
	if (m_captureThread)
	{
		m_captureThread->setResolution(m_cameraResolutions.at(index));
		auto position = m_cameraPositionComboBox->currentIndex();
		if(position != 0)
		{
		  onCameraPositionChanged(position);
		}
		else
		{
		  computeNewPIPPosition();
		}
		m_captureThread->setCameraOverlayPosition(m_PIPposition);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCameraCompositionModeChanged(int index)
{
	if (m_captureThread)
	{
	  auto mode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_compositionComboBox->currentIndex());
		m_captureThread->setCameraOverlayCompositionMode(mode);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onPomodoroCompositionModeChanged(int index)
{
  if (m_captureThread)
  {
    auto mode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_pomodoroCompositionComboBox->currentIndex());
    m_captureThread->setStatisticsOverlayCompositionMode(mode);
  }
}

//-----------------------------------------------------------------
bool DesktopCapture::eventFilter(QObject *object, QEvent *event)
{
  auto drawPomodoro = m_pomodoroGroupBox->isChecked() && m_overlayStats->isChecked();
  auto drawCamera = m_cameraEnabled->isChecked();
	if (!m_captureThread || !(drawCamera || drawPomodoro) || !m_screenshotImage->pixmap())
	{
		return object->eventFilter(object, event);
	}

	static bool insideCamera = false;
	static bool insideStats = false;
	static bool drag = false;

	static QPoint dragPoint = QPoint(0, 0);
	QMouseEvent *me = static_cast<QMouseEvent *>(event);
	Resolution cameraResolution = drawCamera ? m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex()) : Resolution{QString(),0,0};

	QLabel *label = qobject_cast<QLabel *>(object);
	if (!label)
	{
		return false;
	}

	switch (event->type())
	{
		case QEvent::Enter:
			if (m_captureThread)
			{
				m_captureThread->setPaintFrame(true);
			}
			break;
		case QEvent::Leave:
			if (m_captureThread)
			{
				m_captureThread->setPaintFrame(false);
			}
			break;
		case QEvent::MouseButtonPress:
			if (me->button() == Qt::LeftButton)
			{
				auto geometry = captureGeometry();
				auto unusedSpace = m_screenshotImage->size() - m_screenshotImage->pixmap()->size();
				auto ratio = geometry.width() / static_cast<double>(m_screenshotImage->pixmap()->size().width());
				auto mappedPoint = QPoint((me->pos().x() - unusedSpace.width()/2) * ratio, (me->pos().y() - unusedSpace.height()/2) * ratio);

				if ( !drawCamera || (m_PIPposition.x() > mappedPoint.x()) ||
		         (m_PIPposition.y() > mappedPoint.y()) ||
		         (m_PIPposition.x()+cameraResolution.width < mappedPoint.x()) ||
		         (m_PIPposition.y()+cameraResolution.height < mappedPoint.y()) )
				{
					insideCamera = false;
				}
				else
				{
					insideCamera = true;
          m_cameraPositionComboBox->setCurrentIndex(0);
				}

				auto height = ((2 * m_pomodorosNumber->value()) - 1) * CaptureDesktopThread::POMODORO_UNIT_HEIGHT;

				if ( (m_statsPosition.x() > mappedPoint.x()) ||
		         (m_statsPosition.y() > mappedPoint.y()) ||
		         (m_statsPosition.x() + CaptureDesktopThread::POMODORO_UNIT_MAX_WIDTH < mappedPoint.x()) ||
		         (m_statsPosition.y() + height < mappedPoint.y()) )
				{
					insideStats = false;
				}
				else
				{
					insideStats = true;
					m_pomodoroPositionComboBox->setCurrentIndex(0);
				}

				if(!insideCamera && !insideStats)
				{
					break;
				}

				drag = true;
				dragPoint = me->pos();
			}
			break;
		case QEvent::MouseButtonRelease:
			if ((me->button() == Qt::LeftButton) && drag && (insideCamera || insideStats))
			{
				if(insideCamera)
				{
					m_captureThread->setCameraOverlayPosition(computeNewPIPPosition(dragPoint, me->pos()));
				}
				else
				{
					if(insideStats)
					{
						m_captureThread->setStatsOverlayPosition(computeNewStatsPosition(dragPoint, me->pos()));
					}
				}

        drag = false;
				dragPoint = me->pos();
				insideCamera = false;
				insideStats = false;
			}
			break;
		case QEvent::MouseMove:
			if (drag && (insideCamera || insideStats))
			{
				if(insideCamera)
				{
					m_captureThread->setCameraOverlayPosition(computeNewPIPPosition(dragPoint, me->pos()));
				}
				else
				{
					if(insideStats)
					{
						m_captureThread->setStatsOverlayPosition(computeNewStatsPosition(dragPoint, me->pos()));
					}
				}

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
	QRect geometry = captureGeometry();

	double ratioX = static_cast<double>(geometry.width()) / static_cast<double>(imageGeometry.width());
	double ratioY = static_cast<double>(geometry.height()) / static_cast<double>(imageGeometry.height());

	Resolution cameraResolution = m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex());

	int dx = (point.x() - dragPoint.x())*ratioX;
	int dy = (point.y() - dragPoint.y())*ratioY;
	int xLimit = geometry.width() - cameraResolution.width;
	int yLimit = geometry.height() - cameraResolution.height;

	m_PIPposition.setX(m_PIPposition.x() + dx);
	m_PIPposition.setY(m_PIPposition.y() + dy);

	if (m_PIPposition.x() < 0)      m_PIPposition.setX(0);
	if (m_PIPposition.x() > xLimit)	m_PIPposition.setX(xLimit);
	if (m_PIPposition.y() < 0)      m_PIPposition.setY(0);
	if (m_PIPposition.y() > yLimit)	m_PIPposition.setY(yLimit);

	return m_PIPposition;
}

//-----------------------------------------------------------------
QString DesktopCapture::timeToText(const QTime& time) const
{
  QString text;

  if (time.hour() != 0)
  {
    text += QString(" %1 hour").arg(time.hour());

    if (time.hour() > 1)
    {
      text += QString("s");
    }
  }

  if (time.minute() != 0)
  {
    text += QString(" %1 minute").arg(time.minute());

    if (time.minute() > 1)
    {
      text += QString("s");
    }
  }

  if (time.second() != 0)
  {
    text += QString(" %1 second").arg(time.second());

    if (time.second() > 1)
    {
      text += QString("s");
    }
  }

  text += QString(".");

  return text;
}

//-----------------------------------------------------------------
QPoint DesktopCapture::computeNewStatsPosition(const QPoint &dragPoint, const QPoint &point)
{
	QSize imageGeometry = m_screenshotImage->pixmap()->size();
	QRect geometry = captureGeometry();

	double ratioX = static_cast<double>(geometry.width()) / static_cast<double>(imageGeometry.width());
	double ratioY = static_cast<double>(geometry.height()) / static_cast<double>(imageGeometry.height());

	auto height = ((2 * m_pomodorosNumber->value()) - 1) * CaptureDesktopThread::POMODORO_UNIT_HEIGHT;

	int dx = (point.x() - dragPoint.x())*ratioX;
	int dy = (point.y() - dragPoint.y())*ratioY;
	int xLimit = geometry.width() - CaptureDesktopThread::POMODORO_UNIT_MAX_WIDTH;
	int yLimit = geometry.height() - height;

	m_statsPosition.setX(m_statsPosition.x() + dx);
	m_statsPosition.setY(m_statsPosition.y() + dy);

	if (m_statsPosition.x() < 0)		  m_statsPosition.setX(0);
	if (m_statsPosition.x() > xLimit)	m_statsPosition.setX(xLimit);
	if (m_statsPosition.y() < 0)		  m_statsPosition.setY(0);
	if (m_statsPosition.y() > yLimit)	m_statsPosition.setY(yLimit);

	return m_statsPosition;
}

//-----------------------------------------------------------------
void DesktopCapture::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
  QCoreApplication::quit();
}

//-----------------------------------------------------------------
void DesktopCapture::onStartButtonPressed()
{
	if (!m_captureGroupBox->isChecked() && !m_pomodoroGroupBox->isChecked()) return;

	QString trayMessage;
	m_started = true;

	if (m_captureGroupBox->isChecked())
	{
		Q_ASSERT(m_captureThread != nullptr);

		if (!m_captureThread->isPaused())
		{
			m_captureThread->pause();
		}

		auto time = m_screenshotTime->time();
		int ms = time.second() * 1000 + time.minute() * 1000 * 60 + time.hour() * 60 * 60 * 1000 + time.msec();

		m_timer.setInterval(ms);
		m_timer.setSingleShot(false);

		connect(&m_timer, SIGNAL(timeout()),
		        this,     SLOT(capture()), Qt::QueuedConnection);

		QString message = QString("Capture interval set to") + timeToText(time);
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
			message += list[i++] + timeToText(time);
		}

		if (m_captureGroupBox->isChecked())
		{
			trayMessage += QString("\n");
		}

		trayMessage += message;

		if (m_pomodoroAnimateTray->isChecked() && (m_trayIcon != nullptr))
		{
			m_trayIcon->setIcon(QIcon(":/DesktopCapture/0-red.ico"));
		}

		onPomodoroValuesChanged();

		connect(m_pomodoro.get(), SIGNAL(progress(unsigned int)),
		        this,             SLOT(updateTrayProgress()), Qt::DirectConnection);
		connect(m_pomodoro.get(), SIGNAL(pomodoroEnded()),
		        this,             SLOT(trayMessage()), Qt::DirectConnection);
		connect(m_pomodoro.get(), SIGNAL(shortBreakEnded()),
		        this,             SLOT(trayMessage()), Qt::DirectConnection);
		connect(m_pomodoro.get(), SIGNAL(longBreakEnded()),
		        this,             SLOT(trayMessage()), Qt::DirectConnection);
		connect(m_pomodoro.get(), SIGNAL(sessionEnded()),
		        this,             SLOT(trayMessage()), Qt::DirectConnection);

		m_pomodoro->start();
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
		menu->addAction(m_menuAbout);
		menu->addSeparator();
		menu->addAction(m_menuQuit);

		if (m_pomodoroGroupBox->isChecked())
		{
			m_trayIcon->setToolTip(m_pomodoro->statusMessage());
		}
		else
		{
			m_trayIcon->setToolTip(QString("Capturing desktop."));
		}

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

		if(!m_videoRadioButton->isChecked())
		{
			saveCapture(pixmap);
		}
		else
		{
			if (m_secuentialNumber == 0)
			{
				QString fileName = this->m_dirEditLabel->text() + QString("\\DesktopCapture_") + QDateTime::currentDateTime().toString("dd_MM_yyyy") + QString(".webm");
				QRect desktopGeometry = captureGeometry();

				m_vp8_interface = std::make_shared<VPX_Interface>(fileName, desktopGeometry.height(), desktopGeometry.width(), m_fps->value(), m_scale);
			}

			auto image = pixmap->toImage().convertToFormat(QImage::Format_RGB32);
			m_vp8_interface->encodeFrame(&image);
		}
	}

	++m_secuentialNumber;

	if (m_screenshotAnimateTray->isChecked() && (m_trayIcon != nullptr))
	{
		m_trayIcon->setIcon(icon);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCaptureGroupChanged(bool status)
{
  if (status)
  {
    if (!m_captureThread)
    {
      setupCameraResolutions();
      setupCaptureThread();
    }

    if (m_captureThread->isPaused())
    {
      m_captureThread->resume();
    }
  }
  else
  {
    if (m_captureThread && !m_captureThread->isPaused())
    {
      m_captureThread->pause();
    }
  }

  m_startButton->setEnabled(m_pomodoroGroupBox->isChecked() || status);
	m_screenshotImage->setEnabled(status);
	m_screenshotTime->setEnabled(status);
	m_dirButton->setEnabled(status);
	m_dirEditLabel->setEnabled(status);
	m_fps->setEnabled(status);
  m_overlayStats->setEnabled(status);

  auto overlayState = status && m_overlayStats->isChecked();
  m_pomodoroPositionComboBox->setEnabled(overlayState);
  m_pomodoroPositionLabel->setEnabled(overlayState);
  m_pomodoroCompositionComboBox->setEnabled(overlayState);
  m_pomodoroCompositionLabel->setEnabled(overlayState);
}

//-----------------------------------------------------------------
void DesktopCapture::onPomodoroGroupChanged(bool status)
{
	m_startButton->setEnabled(m_captureGroupBox->isChecked() || status);
	m_continuousTicTac->setEnabled(status && m_pomodoroUseSounds->isChecked());
	m_overlayStats->setEnabled(status && m_captureGroupBox->isChecked());
	m_pomodoroPositionComboBox->setEnabled(status && m_captureGroupBox->isChecked());
	m_pomodoroPositionLabel->setEnabled(status && m_captureGroupBox->isChecked());
	m_pomodoroCompositionComboBox->setEnabled(status && m_captureGroupBox->isChecked());
	m_pomodoroCompositionLabel->setEnabled(status && m_captureGroupBox->isChecked());

	if (m_captureThread)
	{
	  auto enabled = status && m_captureGroupBox->isChecked() && this->m_overlayStats->isChecked();
	  std::shared_ptr<Pomodoro> pomodoro = (enabled ? m_pomodoro : nullptr);

		m_captureThread->setPomodoro(pomodoro);
		auto compositionMode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_pomodoroCompositionComboBox->currentIndex());
		m_captureThread->setStatisticsOverlayCompositionMode(compositionMode);

		if(m_pomodoroPositionComboBox->currentIndex() != 0)
		{
		  auto position = static_cast<CaptureDesktopThread::POSITION>(m_pomodoroPositionComboBox->currentIndex());
		  m_captureThread->setStatsOverlayPosition(position);
		}
		else
		{
		  m_captureThread->setStatsOverlayPosition(m_statsPosition);
		}
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCameraPositionChanged(int index)
{
	if (m_captureThread)
	{
	  auto position = static_cast<CaptureDesktopThread::POSITION>(index);
		m_captureThread->setCameraOverlayPosition(position);
		m_PIPposition = m_captureThread->getCameraOverlayPosition();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::updateTrayProgress()
{
	if (m_trayIcon != nullptr)
	{
		m_trayIcon->setIcon(m_pomodoro->icon());
	}
}

//-----------------------------------------------------------------
void DesktopCapture::trayMessage()
{
	QString message;
	QString tooltip;
  QString unit;

	switch(m_pomodoro->status())
	{
    case Pomodoro::Status::LongBreak:
    case Pomodoro::Status::ShortBreak:
      if(m_pomodoro->status() == Pomodoro::Status::LongBreak)
      {
        unit = QString("long");
      }
      else
      {
        unit = QString("short");
      }
      message = QString("Completed a %1 break.\nStarting a pomodoro.").arg(unit);
      tooltip = QString("In a pomodoro.\nCompleted %1 pomodoros.").arg(m_pomodoro->completedPomodoros());
      break;
    case Pomodoro::Status::Pomodoro:
      if((m_pomodoro->completedPomodoros() % m_pomodorosBreakNumber->value()) == 0)
      {
        unit = QString("long");
      }
      else
      {
        unit = QString("short");
      }
      message = QString("Completed a pomodoro.\nStarting a %1 break.").arg(unit);
      tooltip = QString("In a %1 break.\nCompleted %2 pomodoros.").arg(unit).arg(m_pomodoro->completedPomodoros());
      break;
    case Pomodoro::Status::Stopped:
      disconnect(m_pomodoro.get(), SIGNAL(pomodoroEnded()),
                 this,             SLOT(trayMessage()));
      disconnect(m_pomodoro.get(), SIGNAL(shortBreakEnded()),
                 this,             SLOT(trayMessage()));
      disconnect(m_pomodoro.get(), SIGNAL(longBreakEnded()),
                 this,             SLOT(trayMessage()));
      disconnect(m_pomodoro.get(), SIGNAL(sessionEnded()),
                 this,             SLOT(trayMessage()));

      message = QString("Session completed!");
      break;
    default:
    case Pomodoro::Status::Paused:
      break;
	}

	if (m_trayIcon != nullptr)
	{
		m_trayIcon->showMessage(QString("Pomodoro Timer"), message, QSystemTrayIcon::MessageIcon::NoIcon, 1000);
		m_trayIcon->setToolTip(tooltip);
		updateTrayProgress();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onTicTacStateChanged(int status)
{
	bool value = (status == Qt::Checked);
	m_pomodoro->setContinuousTicTac(value);
}

//-----------------------------------------------------------------
void DesktopCapture::onPomodoroSoundsChanged(int status)
{
	bool value = (status == Qt::Checked);
	m_pomodoro->setUseSounds(value);
	m_continuousTicTac->setEnabled(value);
}

//-----------------------------------------------------------------
void DesktopCapture::statisticsDialogClosed()
{
	disconnect(m_statisticsDialog, SIGNAL(finished(int)),
	           this,               SLOT(statisticsDialogClosed()));

	const auto result = m_statisticsDialog->getResult();

	delete m_statisticsDialog;
	m_statisticsDialog = nullptr;

	if (result == PomodoroStatistics::Result::Stop)
	{
		stopCaptureAction();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onTaskButtonPressed()
{
	 changeTask();

	 m_taskEditButton->setDown(false);
}

//-----------------------------------------------------------------
void DesktopCapture::showAction()
{
  if (m_captureThread != nullptr && m_captureGroupBox->isChecked())
  {
    m_captureThread->resume();
  }

  if(m_trayIcon != nullptr)
  {
    clearTrayMenu();
    m_trayIcon->hide();
  }

  show();
  setWindowState(windowState() & (~Qt::WindowMinimized | Qt::WindowActive));
}

//-----------------------------------------------------------------
void DesktopCapture::stopCaptureAction()
{
	stopCapture();

	showAction();
}

//-----------------------------------------------------------------
void DesktopCapture::stopCapture()
{
	m_started = false;

	if (m_captureGroupBox->isChecked())
	{
		disconnect(&m_timer, SIGNAL(timeout()),
		           this,     SLOT(capture()));
	}

	if (m_pomodoroGroupBox->isChecked())
	{
	  disconnect(m_pomodoro.get(), SIGNAL(pomodoroEnded()),
	             this,             SLOT(trayMessage()));
	  disconnect(m_pomodoro.get(), SIGNAL(shortBreakEnded()),
	             this,             SLOT(trayMessage()));
	  disconnect(m_pomodoro.get(), SIGNAL(longBreakEnded()),
	             this,             SLOT(trayMessage()));
	  disconnect(m_pomodoro.get(), SIGNAL(sessionEnded()),
	             this,             SLOT(trayMessage()));
		m_pomodoroTask->setText(m_pomodoro->getTask());
		m_pomodoro->stop();
		m_pomodoro->clear();
	}

	if (m_captureThread && m_captureGroupBox->isChecked())
	{
		if(m_videoRadioButton->isChecked())
		{
			m_vp8_interface = nullptr;
		}

		m_secuentialNumber = 0;
		m_captureThread->resume();
	}

	m_trayIcon->hide();
	m_trayIcon->setIcon(QIcon(":/DesktopCapture/application.ico"));
}

//-----------------------------------------------------------------
void DesktopCapture::clearTrayMenu()
{
  QMenu *menu = m_trayIcon->contextMenu();
  for(auto action: menu->actions())
  {
    menu->removeAction(action);

    if(action->isSeparator())
    {
      delete action;
    }
  }
}

//-----------------------------------------------------------------
void DesktopCapture::showStatistics()
{
	if (m_statisticsDialog == nullptr)
	{
		m_statisticsDialog = new PomodoroStatistics(m_pomodoro, this);

		connect(m_statisticsDialog, SIGNAL(finished(int)),
		        this,               SLOT(statisticsDialogClosed()), Qt::QueuedConnection);

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
				                               m_pomodoro->getTask(), &ok);
	if (ok && !text.isEmpty())
	{
		m_pomodoroTask->setText(text);
		m_pomodoro->setTask(text);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::pausePomodoro()
{
	m_paused = true;
	m_pomodoro->pause();
	showStatistics();
}

//-----------------------------------------------------------------
void DesktopCapture::quitApplication()
{
	m_pomodoro->stop();
	stopCapture();
	QApplication::instance()->quit();
}

//-----------------------------------------------------------------
void DesktopCapture::onStatisticsStateChanged(int status)
{
  auto enabled = status == Qt::Checked;
	if(m_captureThread)
	{
	  std::shared_ptr<Pomodoro> pomodoro = enabled ? m_pomodoro : nullptr;
		m_captureThread->setPomodoro(pomodoro);
	}

	m_pomodoroPositionComboBox->setEnabled(enabled);
	m_pomodoroPositionLabel->setEnabled(enabled);
	m_pomodoroCompositionComboBox->setEnabled(enabled);
	m_pomodoroCompositionLabel->setEnabled(enabled);
}

//-----------------------------------------------------------------
void DesktopCapture::onPomodoroValuesChanged()
{
	m_pomodoro->setPomodoroDuration(m_pomodoroTime->time());
	m_pomodoro->setShortBreakDuration(m_shortBreakTime->time());
	m_pomodoro->setLongBreakDuration(m_longBreakTime->time());
	m_pomodoro->setPomodorosBeforeBreak(m_pomodorosBreakNumber->value());
	m_pomodoro->setSessionPodomodos(m_pomodorosNumber->value());
	m_pomodoro->setTask(m_pomodoroTask->text());

	if(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked() && m_captureThread)
	{
	  if(m_pomodoroPositionComboBox->currentIndex() != 0)
	  {
	    onPomodoroPositionChanged(m_pomodoroPositionComboBox->currentIndex());
	  }
	  else
	  {
	    m_captureThread->setStatsOverlayPosition(computeNewStatsPosition());
	  }
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCaptureVideoChanged(bool unused)
{
  auto enabled = (m_videoRadioButton->isChecked());

  m_fpsLabel      ->setEnabled(enabled);
  m_fps           ->setEnabled(enabled);
}

//-----------------------------------------------------------------
void DesktopCapture::onMaskIndexChanged(int value)
{
  Q_ASSERT(m_captureThread);

  auto mask = static_cast<CaptureDesktopThread::MASK>(value);
  m_captureThread->setMask(mask);
}

//-----------------------------------------------------------------
void DesktopCapture::onFaceTrackingChanged(int status)
{
  Q_ASSERT(m_captureThread);

  auto enabled = (status == Qt::Checked);
  m_captureThread->setTrackFace(enabled);
}

//-----------------------------------------------------------------
void DesktopCapture::onConvertToASCIIChanged(int status)
{
  Q_ASSERT(m_captureThread);

  auto enabled = (status == Qt::Checked);
  m_captureThread->setCameraAsASCII(enabled);
}

//-----------------------------------------------------------------
void DesktopCapture::onAboutButtonPressed()
{
  AboutDialog about;
  about.exec();
}

//-----------------------------------------------------------------
void DesktopCapture::onPomodoroPositionChanged(int index)
{
  if (m_captureThread)
  {
    auto position = static_cast<CaptureDesktopThread::POSITION>(index);
    m_captureThread->setStatsOverlayPosition(position);
    m_statsPosition = m_captureThread->getStatsOverlayPosition();
  }
}

//-----------------------------------------------------------------
const QRect DesktopCapture::captureGeometry() const
{
  QRect geometry;
  if (!m_captureAllMonitors->isChecked())
  {
    geometry = QApplication::desktop()->screenGeometry(m_captureMonitorComboBox->currentIndex());
  }
  else
  {
    geometry = QApplication::desktop()->geometry();
  }

  return geometry;
}

//-----------------------------------------------------------------
void DesktopCapture::onScaleIndexChanged(int value)
{
  switch(value)
  {
    case 0:
      m_scale = 0.5;
      break;
    case 2:
      m_scale = 1.5;
      break;
    case 3:
      m_scale = 2.0;
      break;
    case 1:
    default:
      m_scale = 1.0;
      break;
  }
}
