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
#include "VPXInterface.h"
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
	  void capture();
	  void updatePomodoro(bool status);
	  void updateTrayProgress();
	  void trayMessage();
	  void updateContinuousTicTac(int status);
	  void updateUseSounds(int status);
	  void statisticsDialogClosed();
	  void updateTaskName();
	  void stopCaptureAction();
	  void showStatistics();
	  void changeTask();
	  void pausePomodoro();
	  void quitApplication();
	  void updatePomodoroOverlay(int status);
	  void updatePomodoroValues();

	private:
	  static const QString CAPTURE_ENABLED;
	  static const QString CAPTURE_TIME;
	  static const QString CAPTURE_VIDEO;
	  static const QString CAPTURE_VIDEO_FPS;
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
	  static const QString POMODOROS_LAST_TASK;
	  static const QString POMODOROS_OVERLAY;
	  static const QString POMODOROS_OVERLAY_POSITION;

	  static const QStringList CAPTURE_VIDEO_QUALITY_STRINGS;

	  void loadConfiguration();
	  void saveConfiguration();
	  void setupCameraResolutions();
	  void setupTrayIcon();
	  void stopCapture();
	  void setupCaptureThread();
	  void saveCapture(QPixmap *);
	  QPoint computeNewPIPPosition(const QPoint &dragPoint = QPoint(0,0), const QPoint &point = QPoint(0,0));
	  QPoint computeNewStatsPosition(const QPoint &dragPoint = QPoint(0,0), const QPoint &point = QPoint(0,0));

		QStringList           m_cameraResolutionsNames;
		ResolutionList        m_cameraResolutions;
		Pomodoro              m_pomodoro;
		QPixmap               m_desktopCapture;
		QSystemTrayIcon      *m_trayIcon;
		CaptureDesktopThread *m_captureThread;
		QMutex                m_mutex;
		QPixmap              *m_cameraPixmap;
		QPoint                m_PIPposition;
		QPoint                m_statsPosition;
		QPainter::CompositionMode m_compositionMode;
		QTimer                m_timer;
		unsigned long         m_secuentialNumber;
		bool                  m_started;
		PomodoroStatistics   *m_statisticsDialog;
		VPX_Interface        *m_vp8_interface;

		QAction *m_menuPause;
		QAction *m_menuShowStats;
		QAction *m_menuStopCapture;
		QAction *m_menuChangeTask;
		QAction *m_menuQuit;
		bool m_paused;
};

#endif // DESKTOP_CAPTURE_H_
