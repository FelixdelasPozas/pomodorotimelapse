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
const QString DesktopCapture::APPLICATION_STATE = QString("Application State");
const QString DesktopCapture::OVERLAY_POSITION = QString("Camera Overlay Position");
const QString DesktopCapture::OVERLAY_COMPOSITION_MODE = QString("Camera Overlay Composition Mode");
const QString DesktopCapture::POMODORO_TIME = QString("Pomodoro Time");
const QString DesktopCapture::SHORT_BREAK_TIME = QString("Short Break Time");
const QString DesktopCapture::LONG_BREAK_TIME = QString("Long Break Time");
const QString DesktopCapture::POMODOROS_BEFORE_BREAK = QString("Pomodoros Before A Long Break");
const QString DesktopCapture::POMODOROS_ANIMATED_TRAY_ENABLED = QString("Pomodoro Animated Tray Icon");
const QString DesktopCapture::POMODOROS_USE_SOUNDS = QString("Pomodoro Use Sounds");
const QString DesktopCapture::CAPTURE_TIME = QString("Time Between Captures");
const QString DesktopCapture::CAPTURE_ENABLED = QString("Enable Desktop Capture");
const QString DesktopCapture::POMODORO_ENABLED = QString("Enable Pomodoro");

//-----------------------------------------------------------------
DesktopCapture::DesktopCapture()
: m_trayIcon{nullptr}
, m_captureThread{nullptr}
, m_secuentialNumber{0}
, m_started{true}
{
	setupUi(this);

	loadConfiguration();
	setupTrayIcon();
	if (m_captureGroupBox->isChecked())
	{
		setupMonitors();
		setupCameraResolutions();
		setupCaptureThread();
	}
	connect(m_enableCamera, SIGNAL(stateChanged(int)), this, SLOT(updateCameraResolutionsComboBox(int)), Qt::QueuedConnection);
	connect(m_dirButton, SIGNAL(pressed()), this, SLOT(updateOutputDir()));
	connect(m_cameraResolutionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCameraResolution(int)));
	connect(m_compositionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCameraCompositionMode(int)));
	connect(m_startButton, SIGNAL(pressed()), this, SLOT(startCapture()));
	connect(m_captureGroupBox, SIGNAL(clicked(bool)), this, SLOT(updateCaptureDesktop(bool)));
	connect(m_pomodoroGroupBox, SIGNAL(clicked(bool)), this, SLOT(updatePomodoro(bool)));
	m_screenshotImage->installEventFilter(this);

}

//-----------------------------------------------------------------
DesktopCapture::~DesktopCapture()
{
	if (m_captureThread)
	{
		m_captureThread->abort();
		m_captureThread->wait();
	}
	delete m_captureThread;
}

//-----------------------------------------------------------------
void DesktopCapture::loadConfiguration()
{
	QSettings settings("DesktopCapture.ini", QSettings::IniFormat);

	setWindowTitle("Desktop Capture");
	setWindowIcon(QIcon(":/DesktopCapture/application.ico"));

	if (settings.contains(APPLICATION_GEOMETRY))
		restoreGeometry(settings.value(APPLICATION_GEOMETRY).toByteArray());
	else
		showMaximized();

	if (settings.contains(APPLICATION_STATE))
		this->restoreState(settings.value(APPLICATION_STATE).toByteArray());

	QPoint position(0,0);
	if (settings.contains(OVERLAY_POSITION))
		position = settings.value(OVERLAY_POSITION, QPoint(0,0)).toPoint();
	else
		settings.setValue(OVERLAY_POSITION, position);
	m_PIPposition = position;

	int modeIndex;
	if (settings.contains(OVERLAY_COMPOSITION_MODE))
		modeIndex = settings.value(OVERLAY_COMPOSITION_MODE, 0).toInt();
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

	QTime shortBreak;
	if (settings.contains(SHORT_BREAK_TIME))
		shortBreak = settings.value(SHORT_BREAK_TIME, QTime(0,5,0)).toTime();
	else
	{
		shortBreak = QTime(0,5,0);
		settings.setValue(SHORT_BREAK_TIME, shortBreak);
	}
	m_shortBreakTime->setTime(shortBreak);

	QTime longBreak;
	if (settings.contains(LONG_BREAK_TIME))
		longBreak = settings.value(LONG_BREAK_TIME, QTime(0,15,0)).toTime();
	else
	{
		longBreak = QTime(0,15,0);
		settings.setValue(LONG_BREAK_TIME, longBreak);
	}
	m_longBreakTime->setTime(longBreak);

	int pomodorosBeforeBreak;
	if (settings.contains(POMODOROS_BEFORE_BREAK))
		pomodorosBeforeBreak = settings.value(POMODOROS_BEFORE_BREAK, 4).toInt();
	else
	{
		pomodorosBeforeBreak = 4;
		settings.setValue(POMODOROS_BEFORE_BREAK, pomodorosBeforeBreak);
	}
	m_pomodorosBreakNumber->setValue(pomodorosBeforeBreak);

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

	QTime timeBetweenCaptures;
	if (settings.contains(CAPTURE_TIME))
		timeBetweenCaptures = settings.value(CAPTURE_TIME, QTime(0,0,30)).toTime();
	else
	{
		timeBetweenCaptures = QTime(0,0,30);
		settings.setValue(CAPTURE_TIME, timeBetweenCaptures);
	}
	this->m_screeshotTime->setTime(timeBetweenCaptures);

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
	settings.setValue(CAMERA_ENABLED, m_enableCamera->isChecked());
	settings.setValue(CAMERA_ANIMATED_TRAY_ENABLED, m_screenshotAnimateTray->isChecked());
	settings.setValue(CAMERA_RESOLUTIONS, m_cameraResolutionsNames);
	settings.setValue(ACTIVE_RESOLUTION, m_cameraResolutionComboBox->currentIndex());
	settings.setValue(OVERLAY_POSITION, m_PIPposition);
	settings.setValue(OVERLAY_COMPOSITION_MODE, m_compositionComboBox->currentIndex());
  settings.setValue(APPLICATION_GEOMETRY, saveGeometry());
  settings.setValue(APPLICATION_STATE, saveState());
  settings.setValue(POMODORO_ENABLED, m_pomodoroGroupBox->isChecked());
	settings.setValue(POMODORO_TIME, m_pomodoroTime->time());
	settings.setValue(SHORT_BREAK_TIME, m_shortBreakTime->time());
	settings.setValue(LONG_BREAK_TIME, m_longBreakTime->time());
	settings.setValue(POMODOROS_BEFORE_BREAK, m_pomodorosBreakNumber->value());
  settings.setValue(POMODOROS_ANIMATED_TRAY_ENABLED, m_pomodoroAnimateTray->isChecked());
  settings.setValue(POMODOROS_USE_SOUNDS, m_pomodoroUseSounds->isChecked());
  settings.setValue(CAPTURE_TIME, m_screeshotTime->time());
  settings.setValue(CAPTURE_ENABLED, m_captureGroupBox->isChecked());

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

	bool checked = (capturedMonitor == -1);
	m_captureAllMonitors->setChecked(checked);
	m_captureMonitorComboBox->setEnabled(!checked);

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
		{
			computeNewPosition();
			m_captureThread->setOverlayPosition(m_PIPposition);
			m_captureThread->setCaptureMonitor(m_captureMonitorComboBox->currentIndex());
		}
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

	if (m_captureThread)
		m_captureThread->resume();

	// TODO: mostrar otra cosa si ya ha sido activado.
	show();

	if (m_started)
	{
		disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(takeScreenshot()));
		m_started = false;
	}

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

	m_captureThread = new CaptureDesktopThread(monitor, resolution, m_PIPposition, m_compositionMode, this);

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
		m_captureThread->setResolution(this->m_cameraResolutions.at(status));
		computeNewPosition();
		m_captureThread->setOverlayPosition(m_PIPposition);
	}
}

//-----------------------------------------------------------------
void DesktopCapture::updateCameraCompositionMode(int status)
{
	m_compositionMode = CaptureDesktopThread::COMPOSITION_MODES.at(status);
	if (m_captureThread)
		m_captureThread->setOverlayCompositionMode(m_compositionMode);
}

//-----------------------------------------------------------------
bool DesktopCapture::eventFilter(QObject *object, QEvent *event)
{
	if (!m_captureThread || !m_enableCamera->isChecked() || !m_screenshotImage->pixmap())
		return object->eventFilter(object, event);

	static bool drag = false;
	static QPoint dragPoint = QPoint(0, 0);
	QMouseEvent *me = static_cast<QMouseEvent *>(event);
	Resolution cameraResolution = m_cameraResolutions.at(this->m_cameraResolutionComboBox->currentIndex());

	QLabel *label = qobject_cast<QLabel *>(object);
	Q_ASSERT(label);

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
					break;

				drag = true;
				dragPoint = me->pos();
			}
			break;
		case QEvent::MouseButtonRelease:
			if ((me->button() == Qt::LeftButton) && drag)
			{
				drag = false;
				m_captureThread->setOverlayPosition(computeNewPosition(dragPoint, me->pos()));
				dragPoint = me->pos();
			}
			break;
		case QEvent::MouseMove:
			if (drag)
			{
				m_captureThread->setOverlayPosition(computeNewPosition(dragPoint, me->pos()));
				dragPoint = me->pos();
			}
			break;
		default:
			break;
	}

	return object->eventFilter(object, event);
}

//-----------------------------------------------------------------
QPoint DesktopCapture::computeNewPosition(const QPoint &dragPoint, const QPoint &point)
{
	QSize imageGeometry = m_screenshotImage->pixmap()->size();
	QRect geometry;
	if (!m_captureAllMonitors->isChecked())
		geometry = QApplication::desktop()->screenGeometry(m_captureMonitorComboBox->currentIndex());
	else
		geometry = QApplication::desktop()->geometry();

	double ratioX = static_cast<double>(geometry.width()) / static_cast<double>(imageGeometry.width());
	double ratioY = static_cast<double>(geometry.height()) / static_cast<double>(imageGeometry.height());

	Resolution cameraResolution = m_cameraResolutions.at(this->m_cameraResolutionComboBox->currentIndex());

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
void DesktopCapture::closeEvent(QCloseEvent *event)
{
	saveConfiguration();
  QMainWindow::closeEvent(event);
}

//-----------------------------------------------------------------
void DesktopCapture::startCapture()
{
	QString trayMessage;
	if (m_captureGroupBox->isChecked() || m_pomodoroGroupBox->isChecked())
		setWindowState(windowState() | Qt::WindowMinimized | Qt::WindowActive);
	else
		return;

	if (m_captureGroupBox->isChecked())
	{
		if (m_captureThread && !m_captureThread->isPaused())
		m_captureThread->pause();

		auto time = m_screeshotTime->time();
		int ms = time.second() * 1000 + time.minute() * 1000 * 60 + time.hour() * 60 * 60 * 1000 + time.msec();

		m_timer.setInterval(ms);
		m_timer.setSingleShot(false);
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(takeScreenshot()), Qt::QueuedConnection);
		m_started = true;

		QString message("Interval set to");
		if (time.hour() != 0)
		{
			if (time.hour() > 1)
				message += QString(" %1 hours").arg(time.hour());
			else
				message += QString(" %1 hour").arg(time.hour());
		}
		if (time.minute() != 0)
		{
			if (time.minute() > 1)
				message += QString(" %1 minutes").arg(time.minute());
			else
				message += QString(" %1 minute").arg(time.minute());
		}
		if (time.second() != 0)
		{
			if (time.second() > 1)
				message += QString(" %1 seconds.").arg(time.second());
			else
				message += QString(" %1 second.").arg(time.second());
		}

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
				if (time.hour() > 1)
					message += QString(" %1 hours").arg(time.hour());
				else
					message += QString(" %1 hour").arg(time.hour());
			}
			if (time.minute() != 0)
			{
				if (time.minute() > 1)
					message += QString(" %1 minutes").arg(time.minute());
				else
					message += QString(" %1 minute").arg(time.minute());
			}
			if (time.second() != 0)
			{
				if (time.second() > 1)
					message += QString(" %1 seconds.").arg(time.second());
				else
					message += QString(" %1 second.").arg(time.second());
			}
		}

		if (this->m_captureGroupBox->isChecked())
			trayMessage += QString("\n\n");

		trayMessage += message;
		m_trayIcon->setIcon(QIcon(":/DesktopCapture/0-red.ico"));

		// TODO: start pomodoro
	}

	m_trayIcon->showMessage(QString("Started"), trayMessage, QSystemTrayIcon::MessageIcon::Information, 1000);
}

//-----------------------------------------------------------------
void DesktopCapture::takeScreenshot()
{
	QIcon icon;
	if (m_screenshotAnimateTray->isChecked())
	{
		icon = m_trayIcon->icon();
		m_trayIcon->setIcon(QIcon(":/DesktopCapture/application-shot.ico"));
	}

	if (m_captureThread)
	{
		m_captureThread->takeScreenshot();
		saveCapture(m_captureThread->getImage());
	}
	++m_secuentialNumber;

	if (m_screenshotAnimateTray->isChecked())
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
				setupMonitors();
				setupCameraResolutions();
				setupCaptureThread();
			}

			m_screenshotImage->setEnabled(true);
			m_startButton->setEnabled(true);
			break;
		case false:
			if (m_captureThread && !m_captureThread->isPaused())
				m_captureThread->pause();
			m_screenshotImage->setEnabled(false);
			m_screenshotImage->clear();
			m_startButton->setEnabled(m_pomodoroGroupBox->isChecked());
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------
void DesktopCapture::updatePomodoro(bool status)
{
	m_startButton->setEnabled(m_captureGroupBox->isChecked() || status);
}
