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
#include <DesktopCapture.h>
#include <ProbeResolutionsDialog.h>
#include <CaptureDesktopThread.h>
#include <PomodoroStatistics.h>
#include <AboutDialog.h>
#include <Pomodoro.h>
#include <Utils.h>
#include <VPXInterface.h>

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// Qt
#include <QtGui>
#include <QString>
#include <QPixmap>
#include <QMenu>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>
#include <QFileDialog>

// TEST & TIME

const QStringList COMPOSITION_MODES_NAMES = { "Copy", "Plus", "Multiply" };

const QStringList POSITION_NAMES = { "Free",
                                     "Top Left",
                                     "Top Center",
                                     "Top Right",
                                     "Center Left",
                                      "Center",
                                      "Center Right",
                                      "Bottom Left",
                                      "Bottom Center",
                                      "Bottom Right" };

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
	setWindowIcon(QIcon(":/DesktopCapture/application.svg"));

	m_pomodoro = std::make_shared<Pomodoro>();

	m_config.load();
	applyConfiguration();

	setupTrayIcon();
	setupCameraResolutions();

	if (m_captureGroupBox->isChecked())
	  setupCaptureThread();

	connectSignals();

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
			m_vp8_interface = nullptr;

		m_captureThread->abort();
		m_captureThread->resume();
		m_captureThread->wait();
	}

	m_config.appGeometry = saveGeometry();
	m_config.appState = saveState();
	m_config.save();
}

//-----------------------------------------------------------------
void DesktopCapture::applyConfiguration()
{
  m_config.load();

	if (!m_config.appGeometry.isEmpty())
		restoreGeometry(m_config.appGeometry);
	else
		showMaximized();

	if (!m_config.appState.isEmpty())
		restoreState(m_config.appState);

	if(m_config.monitors.isEmpty())
		m_config.monitors = detectedMonitors();

	m_captureMonitorComboBox->insertItems(0, m_config.monitors);
  m_captureAllMonitors->setChecked(m_config.captureMonitor == -1);
  m_captureMonitorComboBox->setCurrentIndex((m_config.captureMonitor != -1) ? m_config.captureMonitor : 0);
  m_captureMonitorComboBox->setEnabled(!this->m_captureAllMonitors->isChecked());

	m_pomodoroGroupBox->setChecked(m_config.pomodoroEnabled);
	m_pomodoroTime->setTime(m_config.pomodoroTime);
	m_pomodoro->setPomodoroDuration(m_config.pomodoroTime);

	m_shortBreakTime->setTime(m_config.pomodoroShortBreak);
	m_pomodoro->setShortBreakDuration(m_config.pomodoroShortBreak);

	m_longBreakTime->setTime(m_config.pomodoroLongBreak);
	m_pomodoro->setLongBreakDuration(m_config.pomodoroLongBreak);

	m_pomodorosBreakNumber->setValue(m_config.pomodorosBeforeBreak);
	m_pomodoro->setPomodorosBeforeBreak(m_config.pomodorosBeforeBreak);

	m_pomodoroAnimateTray->setChecked(m_config.pomodoroAnimatedIcon);

	m_pomodoroUseSounds->setChecked(m_config.pomodoroUseSounds);
	m_pomodoro->setUseSounds(m_config.pomodoroUseSounds);

	m_continuousTicTac->setChecked(m_config.pomodoroSoundTicTac);
	m_continuousTicTac->setEnabled(m_config.pomodoroUseSounds && this->m_pomodoroGroupBox->isChecked());
	m_pomodoro->setContinuousTicTac(m_config.pomodoroSoundTicTac);

	m_pomodorosNumber->setValue(m_config.pomodorosNumber);
	m_pomodoro->setSessionPodomodos(m_config.pomodorosNumber);

	m_pomodoroTask->setText(m_config.pomodoroTask);
	m_pomodoro->setTaskTitle(m_config.pomodoroTask);

	m_overlayStats->setChecked(m_config.pomodoroOverlay);
	m_overlayStats->setEnabled(m_pomodoroGroupBox->isChecked());

  m_pomodoroPositionComboBox->insertItems(0, POSITION_NAMES);
  m_pomodoroPositionComboBox->setCurrentIndex(m_config.pomodoroOverlayFixedPosition);
  m_pomodoroPositionComboBox->setEnabled(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked());
  m_pomodoroPositionLabel->setEnabled(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked());

  m_pomodoroCompositionComboBox->insertItems(0, COMPOSITION_MODES_NAMES);
  m_pomodoroCompositionComboBox->setCurrentIndex(m_config.pomodoroOverlayCompositionMode);
  m_pomodoroCompositionComboBox->setEnabled(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked());
  m_pomodoroCompositionLabel->setEnabled(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked());

	m_screenshotTime->setTime(m_config.captureTime);

	m_videoRadioButton->setChecked(m_config.captureVideo);
	m_screenshotsRadioButton->setChecked(!m_config.captureVideo);

	m_fps->setEnabled(m_config.captureVideo);
	m_fps->setValue(m_config.captureVideoFPS);

	m_dirEditLabel->setText(m_config.captureOutputDir.replace('/', QDir::separator()));

	onScaleIndexChanged(m_config.captureScale);
  m_scaleComboBox->setCurrentIndex(m_config.captureScale);

	m_captureGroupBox->setChecked(m_config.captureEnabled);

  m_cameraEnabled->setChecked(m_config.cameraEnabled);

  m_compositionComboBox->insertItems(0, COMPOSITION_MODES_NAMES);
  m_compositionComboBox->setCurrentIndex(m_config.cameraOverlayCompositionMode);
  m_compositionComboBox->setEnabled(m_config.cameraEnabled);

  m_cameraPositionComboBox->insertItems(0, POSITION_NAMES);
  m_cameraPositionComboBox->setCurrentIndex(m_config.cameraOverlayFixedPosition);
  m_cameraPositionComboBox->setEnabled(m_config.cameraEnabled);

	QStringList ramps;
	for(const auto &ramp: CaptureDesktopThread::RAMPS)
		ramps << ramp.name;

	m_rampCombo->insertItems(0, ramps);
	m_rampCombo->setCurrentIndex(m_config.cameraASCIIArtRamp);
	m_rampCharacterSize->setValue(std::min(22, std::max(8, m_config.cameraASCIIArtCharacterSize)));

  QStringList masks;
  for(const auto &mask: CaptureDesktopThread::MASKS)
    masks << mask.name;

  masks << QString("None");
  m_cameraMaskComboBox->insertItems(0, masks);
  m_cameraMaskComboBox->setCurrentIndex(m_config.cameraMask);

  m_trackFace->setChecked(m_config.cameraCenterFace);
	m_maskSmooth->setChecked(m_config.cameraFaceSmooth);
	m_maskSmooth->setEnabled(m_config.cameraMask != CaptureDesktopThread::MASKS.size());

  m_ASCIIart->setChecked(m_config.cameraASCIIart);

	m_screenshotAnimateTray->setChecked(m_config.captureAnimateIcon);

  if (!m_config.cameraResolutions.isEmpty())
    m_cameraResolutionsNames = m_config.cameraResolutions;

	m_cameraResolutionsNames.removeAll("");

	m_timeSize->setValue(m_config.timeOverlayTextSize);
	m_timeGroupBox->setChecked(m_config.timeOverlay);
  m_timePositionComboBox->insertItems(0, POSITION_NAMES);	
	m_timePositionComboBox->setCurrentIndex(m_config.timeOverlayFixedPosition);

	computeVideoTime();
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

  connect(m_ASCIIart, SIGNAL(toggled(bool)),
          this,       SLOT(onConvertToASCIIChanged(bool)));

  connect(m_about, SIGNAL(pressed()),
          this,    SLOT(onAboutButtonPressed()));

  connect(m_quit, SIGNAL(clicked()), 
	        this,   SLOT(quitApplication()));					

	connect(m_fps, SIGNAL(valueChanged(int)), 
	        this,  SLOT(computeVideoTime()));

	connect(m_screenshotTime, SIGNAL(timeChanged(QTime)), 
	        this,             SLOT(computeVideoTime()));

	connect(m_rampCombo, SIGNAL(currentIndexChanged(int)), 
	        this,        SLOT(onRampChanged(int)));

	connect(m_rampCharacterSize, SIGNAL(valueChanged(int)), 
	        this,                SLOT(onRampCharSizeChanged(int)));

	connect(m_maskSmooth, SIGNAL(stateChanged(int)), 
	        this,         SLOT(onFaceCoordinatesSmoothChanged(int)));

	connect(m_timeGroupBox, SIGNAL(toggled(bool)), 
	        this,           SLOT(onTimeOverlayStateChanged(bool)));

  connect(m_timePositionComboBox, SIGNAL(currentIndexChanged(int)),
          this,                   SLOT(onTimePositionChanged(int)));

	connect(m_timeSize, SIGNAL(valueChanged(int)), 
	        this,       SLOT(onTimeTextSizeChanged(int)));
}

//-----------------------------------------------------------------
void DesktopCapture::setupCameraResolutions()
{
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
					m_cameraResolutionsNames << getResolutionAsString(resolution);

				m_cameraResolutionComboBox->insertItems(0, m_cameraResolutionsNames);
				m_cameraResolutionComboBox->setCurrentIndex(m_cameraResolutionsNames.size() / 2);

				if (m_captureThread)
				{
					m_captureThread->setCameraResolution(m_cameraResolutions.at(m_config.cameraResolution));
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
		}
	}

  m_cameraResolutionComboBox->setCurrentIndex(std::min(m_config.cameraResolution, m_cameraResolutionComboBox->count()));
	m_cameraResolutionComboBox->setEnabled(m_cameraEnabled->isChecked());

	if (m_captureThread && m_captureGroupBox->isChecked())
		m_captureThread->resume();
}

//-----------------------------------------------------------------
void DesktopCapture::onMonitorsStateChanged(int status)
{
	bool checked = (status == Qt::Checked);
	m_captureMonitorComboBox->setEnabled(!checked);

	if (m_captureThread)
	{
		if (checked)
			m_config.captureMonitor = -1;
		else
			m_config.captureMonitor = m_captureMonitorComboBox->currentIndex();

		m_captureThread->setMonitor(m_config.captureMonitor);
    recomputeOverlaysPositions();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onMonitorValueChanged(int index)
{
	if (m_captureThread)
	{
		m_config.captureMonitor = index;
		m_captureThread->setMonitor(index);
		recomputeOverlaysPositions();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::recomputeOverlaysPositions()
{
  if (m_config.cameraEnabled)
  {
    if(m_cameraPositionComboBox->currentIndex() != 0)
    {
      const auto position = static_cast<CaptureDesktopThread::POSITION>(m_cameraPositionComboBox->currentIndex());
      m_captureThread->setCameraOverlayPosition(position);
			m_config.cameraOverlayFixedPosition = m_cameraPositionComboBox->currentIndex();
      m_config.cameraOverlayPosition = m_captureThread->getCameraOverlayPosition();
    }
    else
    {
      computeNewPIPPosition();
      m_captureThread->setCameraOverlayPosition(m_config.cameraOverlayPosition);
    }
	}

	if(m_config.pomodoroEnabled)
	{
    if(m_pomodoroPositionComboBox->currentIndex() != 0)
    {
      const auto position = static_cast<CaptureDesktopThread::POSITION>(m_pomodoroPositionComboBox->currentIndex());
      m_captureThread->setStatsOverlayPosition(position);
			m_config.pomodoroOverlayFixedPosition = m_pomodoroPositionComboBox->currentIndex();
      m_config.pomodoroOverlayPosition = m_captureThread->getStatsOverlayPosition();
    }
    else
    {
      computeNewStatsPosition();
      m_captureThread->setStatsOverlayPosition(m_config.pomodoroOverlayPosition);
    }
  }

	if(m_config.timeOverlay)
	{
    if(m_pomodoroPositionComboBox->currentIndex() != 0)
    {
      const auto position = static_cast<CaptureDesktopThread::POSITION>(m_timePositionComboBox->currentIndex());
      m_captureThread->setTimeOverlayPosition(position);
			m_config.pomodoroOverlayFixedPosition = m_timePositionComboBox->currentIndex();
      m_config.pomodoroOverlayPosition = m_captureThread->getTimeOverlayPosition();
    }
    else
    {
      computeNewTimePosition();
      m_captureThread->setTimeOverlayPosition(m_config.timeOverlayPosition);
    }
  }	
}

//-----------------------------------------------------------------
QStringList DesktopCapture::detectedMonitors() const
{
	QStringList monitors;
	const auto &screens = QApplication::screens();
	for (int i = 0; i < screens.size(); ++i)
	{
		const auto screen = screens.at(i);
		const auto geometry = screen->geometry();
		if (QApplication::primaryScreen() == screen)
			monitors << QString("Primary Screen (Size: %1x%2 - Position: %3x%4)").arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
		else
			monitors << QString("Additional Screen %1 (Size: %2x%3 - Position: %4x%5)").arg(i).arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
	}

	return monitors;
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
void DesktopCapture::onRampChanged(int index)
{
	m_config.cameraASCIIArtRamp = index;
	m_captureThread->setRamp(index);
}

//-----------------------------------------------------------------
void DesktopCapture::onRampCharSizeChanged(int size)
{
	m_config.cameraASCIIArtCharacterSize = size;
	m_captureThread->setRampCharSize(size);
}

//-----------------------------------------------------------------
void DesktopCapture::onCameraStateChanged(bool enabled)
{
	m_config.cameraEnabled = enabled;

	m_cameraResolutionComboBox->setEnabled(enabled);
	m_cameraPositionComboBox  ->setEnabled(enabled);
	m_compositionComboBox     ->setEnabled(enabled);

	if (m_cameraResolutions.isEmpty() && enabled)
		setupCameraResolutions();

	if (m_captureThread)
		m_captureThread->setCameraEnabled(enabled);
}

//-----------------------------------------------------------------
void DesktopCapture::saveCapture(QPixmap *capture) const
{
	QString format("png");
	QString fileName = m_dirEditLabel->text() + tr("/DesktopCapture_") + QString("%1").arg(m_secuentialNumber,4,'d',0,'0') + QString(".") + format;

	if(m_scale != 1.0)
	  capture->scaled(capture->size() * m_scale, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation).save(fileName, format.toStdString().c_str(), 0);
	else
	  capture->save(fileName, format.toStdString().c_str(), 0);
}

//-----------------------------------------------------------------
void DesktopCapture::setupTrayIcon()
{
	if(!QSystemTrayIcon::isSystemTrayAvailable()) return;

	auto menu = new QMenu(this);
	m_trayIcon = std::make_unique<QSystemTrayIcon>(this);
	m_trayIcon->setIcon(QIcon(":/DesktopCapture/application.svg"));
	m_trayIcon->setContextMenu(menu);

	connect(m_trayIcon.get(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	        this,             SLOT(activateTrayIcon(QSystemTrayIcon::ActivationReason)));

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
			stopCaptureAction();
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

	const auto mask = static_cast<CaptureDesktopThread::MASK>(m_cameraMaskComboBox->currentIndex());
	const auto cameraCompositionMode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_compositionComboBox->currentIndex());

	m_captureThread = std::make_unique<CaptureDesktopThread>(monitor, resolution, this);
	m_captureThread->setCameraOverlayCompositionMode(cameraCompositionMode);
  m_captureThread->setTrackFace(m_trackFace->isChecked());
	m_captureThread->setMask(mask);
	m_captureThread->setRamp(m_rampCombo->currentIndex());
	m_captureThread->setRampCharSize(m_rampCharacterSize->value());
	m_captureThread->setCameraAsASCII(m_ASCIIart->isChecked());

	if(m_cameraPositionComboBox->currentIndex() != 0)
	{
	  const auto position = static_cast<CaptureDesktopThread::POSITION>(m_cameraPositionComboBox->currentIndex());
	  m_captureThread->setCameraOverlayPosition(position);
	  m_config.cameraOverlayPosition = m_captureThread->getCameraOverlayPosition();
	}
	else
	  m_captureThread->setCameraOverlayPosition(m_config.cameraOverlayPosition);

	if (m_pomodoroGroupBox->isChecked())
	{
    const auto statisticsCompositonMode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_config.pomodoroOverlayCompositionMode);

		m_captureThread->setPomodoro(m_pomodoro);
	  m_captureThread->setStatisticsOverlayCompositionMode(statisticsCompositonMode);

	  if(m_pomodoroPositionComboBox->currentIndex() != 0)
	  {
	    const auto position = static_cast<CaptureDesktopThread::POSITION>(m_config.pomodoroOverlayFixedPosition);
	    m_captureThread->setStatsOverlayPosition(position);
	    m_config.pomodoroOverlayPosition = m_captureThread->getStatsOverlayPosition();
	  }
	  else
	    m_captureThread->setStatsOverlayPosition(m_config.pomodoroOverlayPosition);
	}

	m_captureThread->setTimeOverlayEnabled(m_config.timeOverlay);
	m_captureThread->setTimeOverlayTextSize(m_config.timeOverlayTextSize);
	if (m_timePositionComboBox->currentIndex() != 0)
	{
		const auto position = static_cast<CaptureDesktopThread::POSITION>(m_timePositionComboBox->currentIndex());
		m_captureThread->setTimeOverlayPosition(position);
		m_config.timeOverlayPosition = m_captureThread->getTimeOverlayPosition();
	}
	else
		m_captureThread->setTimeOverlayPosition(m_config.timeOverlayPosition);

	connect(m_captureThread.get(), SIGNAL(imageAvailable()),
	        this,                  SLOT(renderImage()), Qt::QueuedConnection);

	m_captureThread->start(QThread::Priority::NormalPriority);
}

//-----------------------------------------------------------------
void DesktopCapture::renderImage()
{
	const auto pixmap = m_captureThread->getImage();
	m_screenshotImage->setPixmap(pixmap->scaled(m_screenshotImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

//-----------------------------------------------------------------
void DesktopCapture::onDirButtonPressed()
{
	QDir dir(m_dirEditLabel->text());
	if (!dir.exists())
		m_dirEditLabel->setText(QDir::currentPath());

	QString dirText = QFileDialog::getExistingDirectory(this, tr("Open Directory"), m_dirEditLabel->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if(!dirText.isEmpty())
	{
		m_config.captureOutputDir = dirText;
	  m_dirEditLabel->setText(dirText.replace('/', QDir::separator()));
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCameraResolutionChanged(int index)
{
	if (m_captureThread)
	{
		m_config.cameraResolution = index;

		m_captureThread->setCameraResolution(m_cameraResolutions.at(index));
		const auto position = m_cameraPositionComboBox->currentIndex();
		if(position != 0)
		  onCameraPositionChanged(position);
		else
		  computeNewPIPPosition();

		m_captureThread->setCameraOverlayPosition(m_config.cameraOverlayPosition);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCameraCompositionModeChanged(int index)
{
	if (m_captureThread)
	{
	  const auto position = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_compositionComboBox->currentIndex());
		m_captureThread->setCameraOverlayCompositionMode(position);
		m_config.cameraOverlayCompositionMode = m_compositionComboBox->currentIndex();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onPomodoroCompositionModeChanged(int index)
{
	m_config.pomodoroOverlayCompositionMode = m_pomodoroCompositionComboBox->currentIndex();

  if (m_captureThread)
  {
    const auto mode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_config.pomodoroOverlayCompositionMode);
    m_captureThread->setStatisticsOverlayCompositionMode(mode);
  }
}

//-----------------------------------------------------------------
bool DesktopCapture::eventFilter(QObject *object, QEvent *event)
{
  const auto drawPomodoro = m_pomodoroGroupBox->isChecked() && m_overlayStats->isChecked();
  const auto drawCamera = m_cameraEnabled->isChecked();
	const auto drawTime = m_timeGroupBox->isChecked();
	if (!m_captureThread || !(drawCamera || drawPomodoro || drawTime) || !m_screenshotImage->pixmap())
	{
		return object->eventFilter(object, event);
	}

	static bool insideCamera = false;
	static bool insideStats = false;
	static bool insideTime = false;
	static bool drag = false;

	static QPoint dragPoint = QPoint(0, 0);
	QMouseEvent *me = static_cast<QMouseEvent *>(event);
	Resolution cameraResolution = drawCamera ? m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex()) : Resolution{QString(),0,0};

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
				const auto geometry = captureGeometry();
				const auto unusedSpace = m_screenshotImage->size() - m_screenshotImage->pixmap().size();
				const auto ratio = geometry.width() / static_cast<double>(m_screenshotImage->pixmap().size().width());
				const auto mappedPoint = QPoint((me->pos().x() - unusedSpace.width()/2) * ratio, (me->pos().y() - unusedSpace.height()/2) * ratio);

				if ( !drawCamera || (m_config.cameraOverlayPosition.x() > mappedPoint.x()) ||
		         (m_config.cameraOverlayPosition.y() > mappedPoint.y()) ||
		         (m_config.cameraOverlayPosition.x()+cameraResolution.width < mappedPoint.x()) ||
		         (m_config.cameraOverlayPosition.y()+cameraResolution.height < mappedPoint.y()) )
				{
					insideCamera = false;
				}
				else
				{
					insideCamera = true;
          m_cameraPositionComboBox->setCurrentIndex(0);
				}

				auto height = ((2 * m_pomodorosNumber->value()) - 1) * CaptureDesktopThread::POMODORO_UNIT_HEIGHT;

				if ( !m_config.pomodoroEnabled || (m_config.pomodoroOverlayPosition.x() > mappedPoint.x()) ||
		         (m_config.pomodoroOverlayPosition.y() > mappedPoint.y()) ||
		         (m_config.pomodoroOverlayPosition.x() + CaptureDesktopThread::POMODORO_UNIT_MAX_WIDTH < mappedPoint.x()) ||
		         (m_config.pomodoroOverlayPosition.y() + height < mappedPoint.y()) )
				{
					insideStats = false;
				}
				else
				{
					insideStats = true;
					m_pomodoroPositionComboBox->setCurrentIndex(0);
				}

				auto timeRect = computeTimeOverlayRect(m_config.timeOverlayTextSize, m_config.timeOverlayPosition);
				if ( !m_config.timeOverlay || (m_config.timeOverlayPosition.x() > mappedPoint.x()) ||
		         (m_config.timeOverlayPosition.y() > mappedPoint.y()) ||
		         (m_config.timeOverlayPosition.x() + timeRect.width() < mappedPoint.x()) ||
		         (m_config.timeOverlayPosition.y() + timeRect.height() < mappedPoint.y()) )
				{
					insideTime = false;
				}
				else
				{
					insideTime = true;
					m_timePositionComboBox->setCurrentIndex(0);
				}				

				if(!insideCamera && !insideStats && !insideTime)
					break;

				drag = true;
				dragPoint = me->pos();
			}
			break;
		case QEvent::MouseButtonRelease:
			if ((me->button() == Qt::LeftButton) && drag && (insideCamera || insideStats || insideTime))
			{
				if(insideCamera)
					m_captureThread->setCameraOverlayPosition(computeNewPIPPosition(dragPoint, me->pos()));
	
				if(insideStats)
					m_captureThread->setStatsOverlayPosition(computeNewStatsPosition(dragPoint, me->pos()));
	
				if(insideTime)						
					m_captureThread->setTimeOverlayPosition(computeNewTimePosition(dragPoint, me->pos()));
	
        drag = false;
				dragPoint = me->pos();
				insideCamera = false;
				insideStats = false;
			}
			break;
		case QEvent::MouseMove:
			if (drag && (insideCamera || insideStats || insideTime))
			{
				if(insideCamera)
					m_captureThread->setCameraOverlayPosition(computeNewPIPPosition(dragPoint, me->pos()));

				if(insideStats)
					m_captureThread->setStatsOverlayPosition(computeNewStatsPosition(dragPoint, me->pos()));
	
				if(insideTime)
					m_captureThread->setTimeOverlayPosition(computeNewTimePosition(dragPoint, me->pos()));
	
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
	const QSize imageGeometry = m_screenshotImage->pixmap().size();
	const QRect geometry = captureGeometry();

	const double ratioX = static_cast<double>(geometry.width()) / static_cast<double>(imageGeometry.width());
	const double ratioY = static_cast<double>(geometry.height()) / static_cast<double>(imageGeometry.height());

	Resolution cameraResolution = m_cameraResolutions.at(m_cameraResolutionComboBox->currentIndex());

	const int dx = (point.x() - dragPoint.x())*ratioX;
	const int dy = (point.y() - dragPoint.y())*ratioY;
	const int xLimit = geometry.width() - cameraResolution.width;
	const int yLimit = geometry.height() - cameraResolution.height;

	m_config.cameraOverlayPosition += QPoint{dx,dy};

	if (m_config.cameraOverlayPosition.x() < 0)       m_config.cameraOverlayPosition.setX(0);
	if (m_config.cameraOverlayPosition.x() > xLimit)	m_config.cameraOverlayPosition.setX(xLimit);
	if (m_config.cameraOverlayPosition.y() < 0)       m_config.cameraOverlayPosition.setY(0);
	if (m_config.cameraOverlayPosition.y() > yLimit)	m_config.cameraOverlayPosition.setY(yLimit);

	return m_config.cameraOverlayPosition;
}

//-----------------------------------------------------------------
QString DesktopCapture::timeToText(const QTime& time) const
{
  QString text;

  if (time.hour() != 0)
    text += QString(" %1 hour%2").arg(time.hour()).arg(time.hour() > 1 ? "s":"");

  if (time.minute() != 0)
    text += QString(" %1 minute%2").arg(time.minute()).arg(time.minute() > 1 ? "s":"");

  if (time.second() != 0)
    text += QString(" %1 second%2").arg(time.second()).arg(time.hour() > 1 ? "s":"");;

  text += QString(".");

  return text;
}

//-----------------------------------------------------------------
QPoint DesktopCapture::computeNewStatsPosition(const QPoint &dragPoint, const QPoint &point)
{
	const QSize imageGeometry = m_screenshotImage->pixmap().size();
	const QRect geometry = captureGeometry();

	const double ratioX = static_cast<double>(geometry.width()) / static_cast<double>(imageGeometry.width());
	const double ratioY = static_cast<double>(geometry.height()) / static_cast<double>(imageGeometry.height());

	const auto height = ((2 * m_pomodorosNumber->value()) - 1) * CaptureDesktopThread::POMODORO_UNIT_HEIGHT;

	const int dx = (point.x() - dragPoint.x())*ratioX;
	const int dy = (point.y() - dragPoint.y())*ratioY;
	const int xLimit = geometry.width() - CaptureDesktopThread::POMODORO_UNIT_MAX_WIDTH;
	const int yLimit = geometry.height() - height;

	m_config.pomodoroOverlayPosition += QPoint{dx,dy};

	if (m_config.pomodoroOverlayPosition.x() < 0)		    m_config.pomodoroOverlayPosition.setX(0);
	if (m_config.pomodoroOverlayPosition.x() > xLimit)	m_config.pomodoroOverlayPosition.setX(xLimit);
	if (m_config.pomodoroOverlayPosition.y() < 0)		    m_config.pomodoroOverlayPosition.setY(0);
	if (m_config.pomodoroOverlayPosition.y() > yLimit)	m_config.pomodoroOverlayPosition.setY(yLimit);

	return m_config.pomodoroOverlayPosition;
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

		if(!QDir{m_dirEditLabel->text()}.exists())
		{
			QMessageBox msgBox{this};
			msgBox.setWindowTitle(tr("Capture Desktop"));
			msgBox.setText(tr("The directory '%1' doesn't exist!").arg(QDir::toNativeSeparators(m_dirEditLabel->text())));
			msgBox.setStandardButtons(QMessageBox::Button::Ok);
			msgBox.setIcon(QMessageBox::Icon::Critical);
			msgBox.exec();

			return;
		}

		if (!m_captureThread->isPaused())
			m_captureThread->pause();

		const auto time = m_screenshotTime->time();
		const int ms = time.second() * 1000 + time.minute() * 1000 * 60 + time.hour() * 60 * 60 * 1000 + time.msec();

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
			message += list[i++] + timeToText(time);

		if (m_captureGroupBox->isChecked())
			trayMessage += QString("\n");

		trayMessage += message;

		if (m_pomodoroAnimateTray->isChecked() && m_trayIcon)
			m_trayIcon->setIcon(QIcon(":/DesktopCapture/0-red.ico"));

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

	if (m_trayIcon)
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
			m_trayIcon->setToolTip(m_pomodoro->statusMessage());
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
	if (m_screenshotAnimateTray->isChecked() && m_trayIcon)
	{
		icon = m_trayIcon->icon();
		m_trayIcon->setIcon(QIcon(":/DesktopCapture/application-shot.svg"));
	}

	if (m_captureThread)
	{
		m_captureThread->takeScreenshot();
		const auto pixmap = m_captureThread->getImage();

		if(!m_videoRadioButton->isChecked())
			saveCapture(pixmap);
		else
		{
			if (m_secuentialNumber == 0)
			{
				const auto fileName = this->m_dirEditLabel->text() + QString("\\DesktopCapture_") + QDateTime::currentDateTime().toString("dd_MM_yyyy") + QString(".webm");
				const auto desktopGeometry = captureGeometry();

				m_vp8_interface = std::make_shared<VPX_Interface>(fileName, desktopGeometry.height(), desktopGeometry.width(), m_fps->value(), m_scale);
			}

			auto image = pixmap->toImage().convertToFormat(QImage::Format_RGB32);
			m_vp8_interface->encodeFrame(&image);
		}
	}

	++m_secuentialNumber;

	if (m_screenshotAnimateTray->isChecked() && m_trayIcon)
		m_trayIcon->setIcon(icon);
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
      m_captureThread->resume();
  }
  else
  {
    if (m_captureThread && !m_captureThread->isPaused())
      m_captureThread->pause();
  }

  m_startButton->setEnabled(m_pomodoroGroupBox->isChecked() || status);
	m_screenshotImage->setEnabled(status);
	m_screenshotTime->setEnabled(status);
	m_dirButton->setEnabled(status);
	m_dirEditLabel->setEnabled(status);
	m_fps->setEnabled(status);

	m_captureTabWidget->setTabEnabled(1, status);
	m_captureTabWidget->setTabEnabled(2, status);

  const auto overlayState = status && m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked();
  m_overlayStats->setEnabled(overlayState);
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
	  const auto enabled = status && m_captureGroupBox->isChecked() && this->m_overlayStats->isChecked();
		m_captureThread->setPomodoro(enabled ? m_pomodoro : nullptr);
		const auto compositionMode = static_cast<CaptureDesktopThread::COMPOSITION_MODE>(m_pomodoroCompositionComboBox->currentIndex());
		m_captureThread->setStatisticsOverlayCompositionMode(compositionMode);

		if(m_pomodoroPositionComboBox->currentIndex() != 0)
		{
		  const auto position = static_cast<CaptureDesktopThread::POSITION>(m_pomodoroPositionComboBox->currentIndex());
		  m_captureThread->setStatsOverlayPosition(position);
		}
		else
		  m_captureThread->setStatsOverlayPosition(m_config.pomodoroOverlayPosition);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCameraPositionChanged(int index)
{
	if (m_captureThread)
	{
	  const auto position = static_cast<CaptureDesktopThread::POSITION>(index);
		m_captureThread->setCameraOverlayPosition(position);
		m_config.cameraOverlayFixedPosition = index;
		m_config.cameraOverlayPosition = m_captureThread->getCameraOverlayPosition();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onTimePositionChanged(int index)
{
	m_config.timeOverlayFixedPosition = index;

	if (m_captureThread)
	{
	  const auto position = static_cast<CaptureDesktopThread::POSITION>(index);
		m_captureThread->setTimeOverlayPosition(position);
		m_config.timeOverlayPosition = m_captureThread->getTimeOverlayPosition();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onTimeTextSizeChanged(int value)
{
	m_config.timeOverlayTextSize = value;

	if(m_captureThread)
	{
		m_captureThread->setTimeOverlayTextSize(value);

		// Text size changed, recompute position.
		if(m_config.timeOverlayFixedPosition != 0)
			onTimePositionChanged(m_config.timeOverlayFixedPosition);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::updateTrayProgress()
{
	if (m_trayIcon)
		m_trayIcon->setIcon(m_pomodoro->icon());
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
        unit = QString("long");
      else
        unit = QString("short");
      message = QString("Completed a %1 break.\nStarting a pomodoro.").arg(unit);
      tooltip = QString("In a pomodoro.\nCompleted %1 pomodoros.").arg(m_pomodoro->completedPomodoros());
      break;
    case Pomodoro::Status::Pomodoro:
      if((m_pomodoro->completedPomodoros() % m_pomodorosBreakNumber->value()) == 0)
        unit = QString("long");
      else
        unit = QString("short");
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

	if (m_trayIcon)
	{
		m_trayIcon->showMessage(QString("Pomodoro Timer"), message, QSystemTrayIcon::MessageIcon::NoIcon, 1000);
		m_trayIcon->setToolTip(tooltip);
		updateTrayProgress();
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onTicTacStateChanged(int status)
{
	m_config.pomodoroSoundTicTac = status == Qt::Checked;
	m_pomodoro->setContinuousTicTac(m_config.pomodoroSoundTicTac);
}

//-----------------------------------------------------------------
void DesktopCapture::onPomodoroSoundsChanged(int status)
{
	m_config.pomodoroUseSounds = status == Qt::Checked;
	m_pomodoro->setUseSounds(m_config.pomodoroUseSounds);
	m_continuousTicTac->setEnabled(m_config.pomodoroUseSounds);
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
		stopCaptureAction();
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
    m_captureThread->resume();

  if(m_trayIcon)
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
		m_pomodoroTask->setText(m_pomodoro->getTaskTitle());
		m_pomodoro->stop();
		m_pomodoro->clear();
	}

	if (m_captureThread && m_captureGroupBox->isChecked())
	{
		if(m_videoRadioButton->isChecked())
			m_vp8_interface = nullptr;

		m_secuentialNumber = 0;
		m_captureThread->resume();
	}

	if(m_trayIcon)
	{
		m_trayIcon->hide();
		m_trayIcon->setIcon(QIcon(":/DesktopCapture/application.svg"));
	}
}

//-----------------------------------------------------------------
void DesktopCapture::clearTrayMenu()
{
	if(m_trayIcon)
	{
		QMenu *menu = m_trayIcon->contextMenu();
		for(auto action: menu->actions())
		{
			menu->removeAction(action);

			if(action->isSeparator())
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
	const QString text = QInputDialog::getText(this,
                                             tr("Enter task name"),
				                                     tr("Task name:"),
				                                     QLineEdit::Normal,
				                                     m_pomodoro->getTaskTitle(), &ok);
	if (ok && !text.isEmpty())
	{
		m_config.pomodoroTask = text;
		m_pomodoroTask->setText(text);
		m_pomodoro->setTaskTitle(text);
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
  const auto enabled = status == Qt::Checked;
	m_config.pomodoroEnabled = enabled;
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
	m_pomodoro->setTaskTitle(m_pomodoroTask->text());

	m_config.pomodoroTime = m_pomodoroTime->time();
	m_config.pomodoroShortBreak = m_shortBreakTime->time();
	m_config.pomodoroLongBreak = m_longBreakTime->time();
	m_config.pomodorosBeforeBreak = m_pomodorosBreakNumber->value();
	m_config.pomodorosNumber = m_pomodorosNumber->value();
	m_config.pomodoroTask = m_pomodoroTask->text();

	if(m_overlayStats->isChecked() && m_pomodoroGroupBox->isChecked() && m_captureThread)
	{
	  if(m_pomodoroPositionComboBox->currentIndex() != 0)
	    onPomodoroPositionChanged(m_pomodoroPositionComboBox->currentIndex());
	  else
	    m_captureThread->setStatsOverlayPosition(computeNewStatsPosition());
	}
}

//-----------------------------------------------------------------
void DesktopCapture::onCaptureVideoChanged(bool unused)
{
	const auto enabled = (m_videoRadioButton->isChecked());
	m_config.captureVideo = enabled;
	m_fpsLabel->setEnabled(enabled);
	m_fps->setEnabled(enabled);
	m_computedTime->setEnabled(enabled);
	m_computedTimeLabel->setEnabled(enabled);
}

//-----------------------------------------------------------------
void DesktopCapture::onMaskIndexChanged(int value)
{
  Q_ASSERT(m_captureThread);

  const auto mask = static_cast<CaptureDesktopThread::MASK>(value);
  m_captureThread->setMask(mask);
	m_config.cameraMask = value;
	m_maskSmooth->setEnabled(m_config.cameraMask != CaptureDesktopThread::MASKS.size());
}

//-----------------------------------------------------------------
void DesktopCapture::onFaceTrackingChanged(int status)
{
  Q_ASSERT(m_captureThread);
	m_config.cameraCenterFace = status == Qt::Checked;
  m_captureThread->setTrackFace(m_config.cameraCenterFace);
}

//-----------------------------------------------------------------
void DesktopCapture::onFaceCoordinatesSmoothChanged(int value)
{
  Q_ASSERT(m_captureThread);
	m_config.cameraFaceSmooth = value == Qt::Checked;
  m_captureThread->setTrackFaceSmooth(m_config.cameraFaceSmooth);
}

//-----------------------------------------------------------------
void DesktopCapture::onConvertToASCIIChanged(bool status)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
  Q_ASSERT(m_captureThread);
	m_config.cameraASCIIart = status;
  m_captureThread->setCameraAsASCII(status);
	QApplication::restoreOverrideCursor();
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
	m_config.pomodoroOverlayFixedPosition = index;

  if (m_captureThread)
  {
    const auto position = static_cast<CaptureDesktopThread::POSITION>(index);
    m_captureThread->setStatsOverlayPosition(position);
    m_config.pomodoroOverlayPosition = m_captureThread->getStatsOverlayPosition();
  }
}

//-----------------------------------------------------------------
QPoint DesktopCapture::computeNewTimePosition(const QPoint &dragPoint, const QPoint &point)
{
	const QSize imageGeometry = m_screenshotImage->pixmap().size();
	const QRect geometry = captureGeometry();

	const double ratioX = static_cast<double>(geometry.width()) / static_cast<double>(imageGeometry.width());
	const double ratioY = static_cast<double>(geometry.height()) / static_cast<double>(imageGeometry.height());

	const int dx = (point.x() - dragPoint.x())*ratioX;
	const int dy = (point.y() - dragPoint.y())*ratioY;
	const auto timeRect = computeTimeOverlayRect(m_config.timeOverlayTextSize, m_config.timeOverlayPosition);
	const int xLimit = geometry.width() - timeRect.width();
	const int yLimit = geometry.height() - timeRect.height();

	m_config.timeOverlayPosition += QPoint{dx,dy};

	if (m_config.timeOverlayPosition.x() < 0)		    m_config.timeOverlayPosition.setX(0);
	if (m_config.timeOverlayPosition.x() > xLimit)	m_config.timeOverlayPosition.setX(xLimit);
	if (m_config.timeOverlayPosition.y() < 0)		    m_config.timeOverlayPosition.setY(0);
	if (m_config.timeOverlayPosition.y() > yLimit)	m_config.timeOverlayPosition.setY(yLimit);

	return m_config.timeOverlayPosition;
}

//-----------------------------------------------------------------
const QRect DesktopCapture::captureGeometry() const
{
  QRect geometry;
  if (!m_captureAllMonitors->isChecked())
    geometry = QApplication::screens().at(m_captureMonitorComboBox->currentIndex())->geometry();
  else
    geometry = QApplication::screens().at(0)->virtualGeometry();

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

	m_config.captureScale = value;
}
