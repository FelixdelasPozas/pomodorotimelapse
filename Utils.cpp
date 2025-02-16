/*
 File: Utils.cpp
 Created on: 28/01/2025
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
#include <Utils.h>

// Qt
#include <QCoreApplication>
#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>

const QString CAPTURE_TIME                       = "Time Between Captures";
const QString CAPTURE_ENABLED                    = "Enable Desktop Capture";
const QString CAPTURE_VIDEO                      = "Capture Video";
const QString CAPTURE_VIDEO_FPS                  = "Capture Video FPS";
const QString CAPTURE_ANIMATED_TRAY_ENABLED      = "Capture Animated Tray Icon";
const QString CAPTURED_MONITOR                   = "Captured Desktop Monitor";
const QString MONITORS_LIST                      = "Monitor Resolutions";
const QString OUTPUT_DIR                         = "Output Directory";
const QString OUTPUT_SCALE                       = "Output Scale";
const QString APPLICATION_GEOMETRY               = "Application Geometry";
const QString APPLICATION_STATE                  = "Application State";
const QString CAMERA_ENABLED                     = "Camera Enabled";
const QString CAMERA_RESOLUTIONS                 = "Available Camera Resolutions";
const QString CAMERA_ACTIVE_RESOLUTION           = "Active Resolution";
const QString CAMERA_OVERLAY_POSITION            = "Camera Overlay Position";
const QString CAMERA_OVERLAY_COMPOSITION_MODE    = "Camera Overlay Composition Mode";
const QString CAMERA_OVERLAY_FIXED_POSITION      = "Camera Overlay Fixed Position";
const QString CAMERA_MASK                        = "Camera Mask";
const QString CAMERA_TRACK_FACE                  = "Center face in camera picture";
const QString CAMERA_TRACK_FACE_SMOOTH           = "Smooth face coordinates interpolation";
const QString CAMERA_ASCII_ART                   = "Convert camera picture to ASCII art";
const QString POMODORO_TIME                      = "Pomodoro Time";
const QString POMODORO_SHORT_BREAK_TIME          = "Short Break Time";
const QString POMODORO_LONG_BREAK_TIME           = "Long Break Time";
const QString POMODOROS_BEFORE_BREAK             = "Pomodoros Before A Long Break";
const QString POMODOROS_ANIMATED_TRAY_ENABLED    = "Pomodoro Animated Tray Icon";
const QString POMODOROS_USE_SOUNDS               = "Pomodoro Use Sounds";
const QString POMODORO_ENABLED                   = "Enable Pomodoro";
const QString POMODOROS_CONTINUOUS_TICTAC        = "Continuous Tic-Tac";
const QString POMODOROS_SESSION_NUMBER           = "Pomodoros In Session";
const QString POMODOROS_LAST_TASK                = "Last task";
const QString POMODOROS_OVERLAY                  = "Overlay Pomodoro Statistics In Capture";
const QString POMODOROS_OVERLAY_POSITION         = "Pomodoro Overlay Position";
const QString POMODOROS_OVERLAY_FIXED_POSITION   = "Pomodoro Overlay Fixed Position";
const QString POMODOROS_OVERLAY_COMPOSITION_MODE = "Pomodoro Overlay Composition Mode";
const QString CAMERA_ASCII_ART_RAMP              = "ASCII Art Character Ramp Index";
const QString CAMERA_ASCII_ART_RAMP_CHAR_SIZE    = "ASCII Art Character Size";
const QString TIME_OVERLAY                       = "Overlay Time";
const QString TIME_OVERLAY_POSITION              = "Time Overlay Position";
const QString TIME_OVERLAY_COMPOSITION_MODE      = "Time Overlay Composition Mode";
const QString TIME_OVERLAY_FIXED_POSITION        = "Time Overlay Fixed Position";
const QString TIME_OVERLAY_TEXT_SIZE             = "Time Overlay Text Size";

const QString INI_FILENAME{"DesktopCapture.ini"};

//-----------------------------------------------------------------------
ClickableHoverLabel::ClickableHoverLabel(QWidget *parent, Qt::WindowFlags f)
: QLabel(parent, f)
{}

//-----------------------------------------------------------------------
ClickableHoverLabel::ClickableHoverLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
: QLabel(text, parent, f)
{}

//-----------------------------------------------------------------------
ClickableHoverLabel::~ClickableHoverLabel()
{};

//-----------------------------------------------------------------------
void Configuration::load()
{
  const auto settings = applicationSettings();

	if (settings->contains(APPLICATION_GEOMETRY))
    appGeometry = settings->value(APPLICATION_GEOMETRY).toByteArray();

	if (settings->contains(APPLICATION_STATE))
		appState = settings->value(APPLICATION_STATE).toByteArray();

	QStringList monitors;
	if (settings->contains(MONITORS_LIST))
		monitors = settings->value(MONITORS_LIST, QStringList()).toStringList();
	else
	{
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
	}

  captureMonitor = settings->value(CAPTURED_MONITOR, -1).toInt();
  captureTime = settings->value(CAPTURE_TIME, QTime(0,0,30)).toTime();
	captureVideo = settings->value(CAPTURE_VIDEO, true).toBool();
  captureVideoFPS = settings->value(CAPTURE_VIDEO_FPS, 15).toInt();
  captureOutputDir = settings->value(OUTPUT_DIR, QDir::homePath()).toString();
  captureScale = settings->value(OUTPUT_SCALE, 1).toInt();
  captureEnabled = settings->value(CAPTURE_ENABLED, true).toBool();
  captureAnimateIcon = settings->value(CAPTURE_ANIMATED_TRAY_ENABLED, true).toBool();

  pomodoroEnabled = settings->value(POMODORO_ENABLED, true).toBool();
  pomodoroTime = settings->value(POMODORO_TIME, QTime(0,25,0)).toTime();
  pomodoroShortBreak = settings->value(POMODORO_SHORT_BREAK_TIME, QTime(0,5,0)).toTime();
  pomodoroLongBreak = settings->value(POMODORO_LONG_BREAK_TIME, QTime(0,15,0)).toTime();
  pomodorosBeforeBreak = settings->value(POMODOROS_BEFORE_BREAK, 4).toInt();
  pomodoroAnimatedIcon = settings->value(POMODOROS_ANIMATED_TRAY_ENABLED, true).toBool();
  pomodoroUseSounds = settings->value(POMODOROS_USE_SOUNDS, true).toBool();
  pomodoroSoundTicTac = settings->value(POMODOROS_CONTINUOUS_TICTAC, false).toBool();
  pomodorosNumber = settings->value(POMODOROS_SESSION_NUMBER, 12).toUInt();
  pomodoroTask = settings->value(POMODOROS_LAST_TASK, QString("Undefined task")).toString();
  pomodoroOverlay = settings->value(POMODOROS_OVERLAY, true).toBool();
  pomodoroOverlayPosition = settings->value(POMODOROS_OVERLAY_POSITION, QPoint(0,0)).toPoint();
  pomodoroOverlayFixedPosition = settings->value(POMODOROS_OVERLAY_FIXED_POSITION, 0).toInt();
  pomodoroOverlayCompositionMode = settings->value(POMODOROS_OVERLAY_COMPOSITION_MODE, 0).toInt();

  cameraEnabled = settings->value(CAMERA_ENABLED, false).toBool();
  cameraOverlayPosition = settings->value(CAMERA_OVERLAY_POSITION, QPoint(0,0)).toPoint();
  cameraOverlayCompositionMode = settings->value(CAMERA_OVERLAY_COMPOSITION_MODE, 0).toInt();
  cameraOverlayFixedPosition = settings->value(CAMERA_OVERLAY_FIXED_POSITION, 0).toInt();
  cameraASCIIArtRamp = settings->value(CAMERA_ASCII_ART_RAMP, 0).toInt();
	cameraASCIIArtCharacterSize = settings->value(CAMERA_ASCII_ART_RAMP_CHAR_SIZE, 8).toInt();
  cameraMask = settings->value(CAMERA_MASK, 0).toInt();
  cameraCenterFace = settings->value(CAMERA_TRACK_FACE, false).toBool();
  cameraFaceSmooth = settings->value(CAMERA_TRACK_FACE_SMOOTH, true).toBool();
  cameraASCIIart = settings->value(CAMERA_ASCII_ART, false).toBool();
  cameraResolution = settings->value(CAMERA_ACTIVE_RESOLUTION, 0).toInt();

  if (settings->contains(CAMERA_RESOLUTIONS))
    cameraResolutions = settings->value(CAMERA_RESOLUTIONS, QStringList()).toStringList();

  timeOverlay = settings->value(TIME_OVERLAY, false).toBool();
  timeOverlayPosition = settings->value(TIME_OVERLAY_POSITION, QPoint{0,0}).toPoint();
  timeOverlayFixedPosition = settings->value(TIME_OVERLAY_FIXED_POSITION, 0).toInt();
  timeOverlayTextSize = settings->value(TIME_OVERLAY_TEXT_SIZE, 40).toInt();
}

//-----------------------------------------------------------------------
void Configuration::save()
{
  auto settings = applicationSettings();

  settings->setValue(APPLICATION_GEOMETRY, appGeometry);
  settings->setValue(APPLICATION_STATE, appState);
	settings->setValue(MONITORS_LIST, monitors);

	settings->setValue(CAPTURED_MONITOR, captureMonitor);
  settings->setValue(CAPTURE_TIME, captureTime);
  settings->setValue(CAPTURE_ENABLED, captureEnabled);
  settings->setValue(CAPTURE_VIDEO, captureVideo);
  settings->setValue(CAPTURE_VIDEO_FPS, captureVideoFPS);
	settings->setValue(OUTPUT_DIR, captureOutputDir);
	settings->setValue(OUTPUT_SCALE, captureScale);
	settings->setValue(CAPTURE_ANIMATED_TRAY_ENABLED, captureAnimateIcon);

	settings->setValue(CAMERA_ENABLED, cameraEnabled);
	settings->setValue(CAMERA_RESOLUTIONS, cameraResolutions);
	settings->setValue(CAMERA_ACTIVE_RESOLUTION, cameraResolution);
	settings->setValue(CAMERA_OVERLAY_POSITION, cameraOverlayPosition);
	settings->setValue(CAMERA_OVERLAY_COMPOSITION_MODE, cameraOverlayCompositionMode);
  settings->setValue(CAMERA_OVERLAY_FIXED_POSITION, cameraOverlayFixedPosition);
  settings->setValue(CAMERA_MASK, cameraMask);
  settings->setValue(CAMERA_TRACK_FACE, cameraCenterFace);
  settings->setValue(CAMERA_TRACK_FACE_SMOOTH, cameraFaceSmooth);
  settings->setValue(CAMERA_ASCII_ART, cameraASCIIart);
	settings->setValue(CAMERA_ASCII_ART_RAMP, cameraASCIIArtRamp);
	settings->setValue(CAMERA_ASCII_ART_RAMP_CHAR_SIZE, cameraASCIIArtCharacterSize);

  settings->setValue(POMODORO_ENABLED, pomodoroEnabled);
	settings->setValue(POMODORO_TIME, pomodoroTime);
	settings->setValue(POMODORO_SHORT_BREAK_TIME, pomodoroShortBreak);
	settings->setValue(POMODORO_LONG_BREAK_TIME, pomodoroLongBreak);
	settings->setValue(POMODOROS_BEFORE_BREAK, pomodorosBeforeBreak);
  settings->setValue(POMODOROS_ANIMATED_TRAY_ENABLED, pomodoroAnimatedIcon);
  settings->setValue(POMODOROS_USE_SOUNDS, pomodoroUseSounds);
  settings->setValue(POMODOROS_CONTINUOUS_TICTAC, pomodoroSoundTicTac);
	settings->setValue(POMODOROS_SESSION_NUMBER, pomodorosNumber);
  settings->setValue(POMODOROS_LAST_TASK, pomodoroTask);
  settings->setValue(POMODOROS_OVERLAY, pomodoroOverlay);
  settings->setValue(POMODOROS_OVERLAY_POSITION, pomodoroOverlayPosition);
  settings->setValue(POMODOROS_OVERLAY_FIXED_POSITION, pomodoroOverlayFixedPosition);
  settings->setValue(POMODOROS_OVERLAY_COMPOSITION_MODE, pomodoroOverlayCompositionMode);

  settings->setValue(TIME_OVERLAY, timeOverlay);
  settings->setValue(TIME_OVERLAY_POSITION, timeOverlayPosition);
  settings->setValue(TIME_OVERLAY_FIXED_POSITION, timeOverlayFixedPosition);
  settings->setValue(TIME_OVERLAY_TEXT_SIZE, timeOverlayTextSize);


	settings->sync();
}

//-----------------------------------------------------------------
std::unique_ptr<QSettings> applicationSettings()
{
  QDir applicationDir{QCoreApplication::applicationDirPath()};
  if(applicationDir.exists(INI_FILENAME))
  {
    return std::make_unique<QSettings>(INI_FILENAME, QSettings::IniFormat);
  }

  return std::make_unique<QSettings>("Felix de las Pozas Alvarez", "DesktopCapture");
}

//-----------------------------------------------------------------
QRect computeTimeOverlayRect(int pixelSize, const QPoint &p)
{
	const auto timeText = QDateTime::currentDateTime().time().toString("hh:mm:ss");
	QFont font;
	font.setBold(true);
	font.setPixelSize(pixelSize);
	QFontMetrics fm(font);
	auto timeRect = fm.boundingRect(timeText);
  timeRect.setWidth(timeRect.width()*1.3);
  timeRect.setHeight(timeRect.height()*1.1);
  const auto bottomRight = QPoint{p.x()+timeRect.width(), p.y()+timeRect.height()};
  timeRect.setTopLeft(p);
  timeRect.setBottomRight(bottomRight);

	return timeRect;
}