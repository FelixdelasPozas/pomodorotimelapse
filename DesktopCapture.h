/*
    File: DesktopCapture.h
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

#ifndef DESKTOP_CAPTURE_H_
#define DESKTOP_CAPTURE_H_

// Project
#include "CaptureDesktopThread.h"
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

// C++
#include <memory>

class QAction;
class QMenu;
class QPlainTextEdit;
class QProgressDialog;
class QResizeEvent;
class QEvent;
class QPixmap;

class CaptureDesktopThread;

/** \class DesktopCapture
 *  \brief Main window class.
 *
 */
class DesktopCapture
: public QMainWindow
, public Ui_MainWindow
{
	Q_OBJECT

	public:
	  /** \brief DesktopCapture class constructor.
	   *
	   */
		DesktopCapture();

		/** \brief DesktopCapture class virtual destructor.
		 *
		 */
		~DesktopCapture();

		void changeEvent(QEvent*);
		bool eventFilter(QObject *object, QEvent *event);
		void closeEvent(QCloseEvent *e);

	private slots:
	  /** \brief Updates the captured monitor.
	   * \param[in] index index of the monitor combo box.
	   *
	   */
	  void onMonitorValueChanged(int index);

	  /** \brief Updates the captured monitor to all monitors or the specified by the combobox.
	   * \param[in] status check status value.
	   *
	   */
	  void onMonitorsStateChanged(int status);

	  /** \brief Enables/disables the capture of the camera.
	   * \param[in] status check status value.
	   *
	   */
	  void onCameraStateChanged(int status);

	  /** \brief Updates the position of the camera in the captured image when the value of the combo box changes.
	   * \param[in] index current index of the combo box.
	   *
	   */
	  void onCameraPositionChanged(int status);

	  /** \brief Updates the output directory.
	   *
	   */
	  void onDirButtonPressed();

	  /** \brief Enables/disables the desktop capture.
	   * \param[in] status check status value.
	   *
	   */
	  void onCaptureGroupChanged(bool status);

	  /** \brief Activates the tray icon
	   * \param[in] reason
	   *
	   */
	  void activateTrayIcon(QSystemTrayIcon::ActivationReason reason);

	  /** \brief Renders the captured image on the screen.
	   *
	   */
	  void renderImage();

	  /** \brief Updates the camera resolution.
	   * \param[in] index resolution combo box index.
	   *
	   */
	  void onCameraResolutionChanged(int index);

	  /** \brief Updates the camera composition mode.
	   * \param[in] index composition combo box index.
	   *
	   */
	  void onCameraCompositionModeChanged(int index);

	  /** \brief Minimizes the app and starts the capture to disk.
	   *
	   */
	  void onStartButtonPressed();

	  /** \brief Captures the desktop and saves result to the video/picture.
	   *
	   */
	  void capture();

	  /** \brief Updates the pomodoro.
	   * \param[in] status check status value.
	   *
	   */
	  void onPomodoroGroupChanged(bool status);

	  /** \brief Updates the tray icon with the progress of the pomodoro.
	   *
	   */
	  void updateTrayProgress();

	  /** \brief Shows the tray message.
	   *
	   */
	  void trayMessage();

	  /** \brief Updates the continuous tic-tac value.
	   * \param[in] status check status value.
	   *
	   */
	  void onTicTacStateChanged(int status);

    /** \brief Updates the sounds of the pomodoro.
     * \param[in] status check status value.
     *
     */
	  void onPomodoroSoundsChanged(int status);

	  /** \brief Handles the closing of the statistics dialog.
	   *
	   */
	  void statisticsDialogClosed();

	  /** \brief Updates the task name in the pomodoro.
	   *
	   */
	  void onTaskButtonPressed();

	  /** \brief Stops the capture thread and show the main window.
	   *
	   */
	  void stopCaptureAction();

	  /**  \brief Restores the aplication window from tray icon.
	   *
	   */
	  void showAction();

	  /** \brief Shows the statistics dialog.
	   *
	   */
	  void showStatistics();

	  /** \brief Asks for the new task name and changes the value in the pomodoro and the interface.
	   *
	   */
	  void changeTask();

	  /** \brief Pauses the pomodoro.
	   *
	   */
	  void pausePomodoro();

	  /** \brief Exits the application.
	   *
	   */
	  void quitApplication();

	  /** \brief Updates the pomodoro overlay.
	   * \param[in] status check status value.
	   *
	   */
	  void onStatisticsStateChanged(int status);

	  /** \brief Updates the pomodoro values.
	   *
	   */
	  void onPomodoroValuesChanged();

	  /** \brief updates the GUI when capture video check box changes.
	   * * \param[in] status check status value.
	   *
	   */
	  void onCaptureVideoChanged(int status);

	  /** \brief sets the internal scale value.
     * \param[in] value combo box index.
     *
     */
    void onScaleIndexChanged(int value);

    /** \brief Changes the drawing mask over the detected face.
     * \param[in] value combo box index.
     *
     */
    void onMaskIndexChanged(int value);

    /** \brief Updates the face traking value in the capture thread.
     * \param[in] status check status value.
     *
     */
    void onFaceTrackingChanged(int status);

    /** \brief Shows the about dialog.
     *
     */
    void onAboutButtonPressed();

    /** \brief Updates the position of the pomodoro overlay.
     * \param[in] index combo box index.
     *
     */
    void onPomodoroPositionChanged(int index);

    /** \brief Updates the camera composition mode.
     * \param[in] index composition combo box index.
     *
     */
    void onPomodoroCompositionModeChanged(int index);


	private:
	  static const QStringList COMPOSITION_MODES_NAMES;
	  static const QStringList POSITION_NAMES;
	  static const QString CAPTURE_ENABLED;
	  static const QString CAPTURE_TIME;
	  static const QString CAPTURE_VIDEO;
	  static const QString CAPTURE_VIDEO_FPS;
	  static const QString CAPTURED_MONITOR;
	  static const QString MONITORS_LIST;
	  static const QString OUTPUT_DIR;
	  static const QString OUTPUT_SCALE;
	  static const QString APPLICATION_GEOMETRY;
	  static const QString APPLICATION_STATE;
	  static const QString CAMERA_ENABLED;
	  static const QString CAMERA_ANIMATED_TRAY_ENABLED;
	  static const QString CAMERA_RESOLUTIONS;
	  static const QString CAMERA_ACTIVE_RESOLUTION;
	  static const QString CAMERA_OVERLAY_POSITION;
	  static const QString CAMERA_OVERLAY_COMPOSITION_MODE;
	  static const QString CAMERA_OVERLAY_FIXED_POSITION;
	  static const QString CAMERA_MASK;
	  static const QString CAMERA_TRACK_FACE;
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
	  static const QString POMODOROS_OVERLAY_FIXED_POSITION;
	  static const QString POMODOROS_OVERLAY_COMPOSITION_MODE;

	  static const QStringList CAPTURE_VIDEO_QUALITY_STRINGS;
	  static const QString INI_FILENAME;

	  /** \brief Returns the time in text.
	   * \param[in] time
	   *
	   */
	  QString timeToText(const QTime &time) const;

	  /** \brief Loads the configuration from the ini file.
	   *
	   */
	  void loadConfiguration();

	  /** \brief Saves the configuration to the ini file.
	   *
	   */
	  void saveConfiguration();

	  /** \brief Setups the camera resolutions and launches the probe resolutions thread if necessary.
	   *
	   */
	  void setupCameraResolutions();

	  /** \brief Setups the tray icon menu.
	   *
	   */
	  void setupTrayIcon();

	  /** \brief Stops the capture thread, disconnects the pomodoro and capture thread signals and deletes the tray menu items.
	   *
	   */
	  void stopCapture();

	  /** \brief Setups and launches the capture thread.
	   *
	   */
	  void setupCaptureThread();

	  /** \brief Saves given image to disk.
	   * \param[in] picture pointer.
	   *
	   */
	  void saveCapture(QPixmap *picture) const;

	  /** \brief Computes the "picture in picture" position of the camera in the captured image.
	   * \param[in] dragPoint initial drag point.
	   * \param[in] point actual drag point.
	   *
	   */
	  QPoint computeNewPIPPosition(const QPoint &dragPoint = QPoint(0,0), const QPoint &point = QPoint(0,0));

    /** \brief Computes the "picture in picture" position of the pomodoro statistics in the captured image.
     * \param[in] dragPoint initial drag point.
     * \param[in] point actual drag point.
     *
     */
	  QPoint computeNewStatsPosition(const QPoint &dragPoint = QPoint(0,0), const QPoint &point = QPoint(0,0));

	  /** \brief Returns the geometry of the captured area.
	   *
	   */
	  const QRect captureGeometry() const;

	  /** \brief Connects the GUI signals to it's slots.
	   *
	   */
	  void connectSignals() const;

	  /** \brief Empties the menu actions of the tray menu.
	   *
	   */
	  void clearTrayMenu();

		QStringList                            m_cameraResolutionsNames;
		ResolutionList                         m_cameraResolutions;
		int                                    m_selectedResolution;
		std::shared_ptr<Pomodoro>              m_pomodoro;
		QPixmap                                m_desktopCapture;
		QSystemTrayIcon                       *m_trayIcon;
		CaptureDesktopThread                  *m_captureThread;
		QMutex                                 m_mutex;
		QPoint                                 m_PIPposition;
		QPoint                                 m_statsPosition;
//		CaptureDesktopThread::COMPOSITION_MODE m_compositionMode;
		QTimer                                 m_timer;
		unsigned long                          m_secuentialNumber;
		bool                                   m_started;
		PomodoroStatistics                    *m_statisticsDialog;
		std::shared_ptr<VPX_Interface>         m_vp8_interface;
		float                                  m_scale;

		QAction *m_menuPause;
		QAction *m_menuShowStats;
		QAction *m_show;
		QAction *m_menuStopCapture;
		QAction *m_menuChangeTask;
		QAction *m_menuAbout;
		QAction *m_menuQuit;

		bool m_paused;
};

#endif // DESKTOP_CAPTURE_H_
