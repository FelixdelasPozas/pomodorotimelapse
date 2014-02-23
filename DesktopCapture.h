/*
 * DesktopCapture.h
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef DESKTOP_CAPTURE_H_
#define DESKTOP_CAPTURE_H_

// Application
#include "Resolutions.h"
#include "Pomodoro.h"
#include "PomodoroStatistics.h"
#include "ui_MainWindow.h"

// Qt
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QPainter>
#include <QMutex>

class QAction;
class QMenu;
class QPlainTextEdit;
class QProgressDialog;
class QResizeEvent;
class QEvent;
class QPixmap;

class CaptureDesktopThread;

class DesktopCapture
: public QMainWindow
, public Ui_MainWindow
{
	Q_OBJECT

	public:
		DesktopCapture();
		~DesktopCapture();

		void changeEvent(QEvent*);
		bool eventFilter(QObject *object, QEvent *event);
		void closeEvent(QCloseEvent *e);

	private slots:
	  void updateMonitorsComboBox(int status);
	  void updateMonitorsCheckBox(int status);
	  void updateCameraResolutionsComboBox(int status);
	  void updateCameraPositionComboBox(int status);
	  void updateOutputDir();
	  void updateCaptureDesktop(bool status);
	  void activateTrayIcon(QSystemTrayIcon::ActivationReason);
	  void renderImage();
	  void updateCameraResolution(int status);
	  void updateCameraCompositionMode(int status);
	  void startCapture();
	  void takeScreenshot();
	  void updatePomodoro(bool status);
	  void updateTrayProgress(unsigned int status);
	  void trayMessage();
	  void updateContinuousTicTac(int status);
	  void updateUseSounds(int status);

	private:
	  static const QString CAPTURE_ENABLED;
	  static const QString CAPTURE_TIME;
	  static const QString CAPTURED_MONITOR;
	  static const QString MONITORS_LIST;
	  static const QString OUTPUT_DIR;
	  static const QString APPLICATION_GEOMETRY;
	  static const QString APPLICATION_STATE;
	  static const QString CAMERA_ENABLED;
	  static const QString CAMERA_ANIMATED_TRAY_ENABLED;
	  static const QString CAMERA_RESOLUTIONS;
	  static const QString CAMERA_ACTIVE_RESOLUTION;
	  static const QString CAMERA_OVERLAY_POSITION;
	  static const QString CAMERA_OVERLAY_COMPOSITION_MODE;
	  static const QString CAMERA_OVERLAY_FIXED_POSITION;
	  static const QString POMODORO_ENABLED;
	  static const QString POMODORO_TIME;
	  static const QString POMODORO_SHORT_BREAK_TIME;
	  static const QString POMODORO_LONG_BREAK_TIME;
	  static const QString POMODOROS_BEFORE_BREAK;
	  static const QString POMODOROS_ANIMATED_TRAY_ENABLED;
	  static const QString POMODOROS_USE_SOUNDS;
	  static const QString POMODOROS_CONTINUOUS_TICTAC;
	  static const QString POMODOROS_SESSION_NUMBER;

	  void loadConfiguration();
	  void saveConfiguration();
	  void setupCameraResolutions();
	  void setupMonitors();
	  void setupTrayIcon();
	  void setupCaptureThread();
	  void saveCapture(QPixmap *);
	  QPoint computeNewPosition(const QPoint &dragPoint = QPoint(0,0), const QPoint &point = QPoint(0,0));

		QStringList           m_cameraResolutionsNames;
		ResolutionList        m_cameraResolutions;
		Pomodoro              m_pomodoro;
		QPixmap               m_desktopCapture;
		QSystemTrayIcon      *m_trayIcon;
		CaptureDesktopThread *m_captureThread;
		QMutex                m_mutex;
		QPixmap              *m_cameraPixmap;
		QPoint                m_PIPposition;
		QPainter::CompositionMode m_compositionMode;
		QTimer                m_timer;
		int                   m_secuentialNumber;
		bool                  m_started;
		PomodoroStatistics   *m_statisticsDialog;
};

#endif // DESKTOP_CAPTURE_H_
