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
#include <CaptureDesktopThread.h>
#include <Resolutions.h>
#include <PomodoroStatistics.h>
#include <Utils.h>
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
class QSettings;

class CaptureDesktopThread;
class Pomodoro;
class VPX_Interface;

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

		virtual bool eventFilter(QObject *object, QEvent *event) override;
		
	protected:
		virtual void changeEvent(QEvent*) override;
		virtual void closeEvent(QCloseEvent *e) override;

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
	   * \param[in] enabled boolean value.
	   *
	   */
	  void onCameraStateChanged(bool enabled);

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

	  /** \brief updates the GUI when capture video radio button or screenshots radio button changes state.
     * \param[in] unused check status value.
     *
	   */
	  void onCaptureVideoChanged(bool unused);

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

		/** \brief Activates/Deactivates face coordinates smoothing. 
		 * \param[in] value QCheckbox value.
		 */
		void onFaceCoordinatesSmoothChanged(int value);

    /** \brief Updates the conversion to ASCII art of the camera picture.
     * \param[in] bool check status value.
     *
     */
    void onConvertToASCIIChanged(bool status);

    /** \brief Shows the about dialog.
     *
     */
    void onAboutButtonPressed();

    /** \brief Updates the position of the pomodoro overlay.
     * \param[in] index combo box index.
     *
     */
    void onPomodoroPositionChanged(int index);

    /** \brief Updates the position of the time overlay.
     * \param[in] index combo box index.
     *
     */
    void onTimePositionChanged(int index);

		/** \brief Updates the time overlay text size. 
		 * \param[in] value Pixel size value.
		 * 
		 */
		void onTimeTextSizeChanged(int value);

    /** \brief Updates the camera composition mode.
     * \param[in] index composition combo box index.
     *
     */
    void onPomodoroCompositionModeChanged(int index);

		/** \brief Computes the video length per second.
		 *
		 */
		void computeVideoTime() const;

		/** \brief Updates the character ramp used in the ASCII art.
		 * \param[in] index Index of the ramp in the RAMPS list. 
		 *
		 */
		void onRampChanged(int index);

		/** \brief Updates the character size in the ASCII art. 
		 * \param[in] size Character pixel size.
		 */
		void onRampCharSizeChanged(int size);

	private:
	  /** \brief Returns the time in text.
	   * \param[in] time
	   *
	   */
	  QString timeToText(const QTime &time) const;

	  /** \brief Applies the configuration to the UI.
	   *
	   */
	  void applyConfiguration();

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

    /** \brief Computes the position of the time overlayin the captured image.
     * \param[in] dragPoint initial drag point.
     * \param[in] point actual drag point.
     *
     */
		QPoint computeNewTimePosition(const QPoint &dragPoint = QPoint(0,0), const QPoint &point = QPoint(0,0));

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

	  /** \brief Computes new positions for the overlays.
	   *
	   */
	  void recomputeOverlaysPositions();

		/** \brief Helper method that returns the list of monitors as string.
		 *
		 */
		QStringList detectedMonitors() const;

		QStringList                           m_cameraResolutionsNames; /** camera resolution strings.                   */
		ResolutionList                        m_cameraResolutions;      /** camera resolution structs.                   */
		std::shared_ptr<Pomodoro>             m_pomodoro;               /** pomodoro object.                             */
		QPixmap                               m_desktopCapture;         /** pixmap of captured desktop.                  */
		std::unique_ptr<QSystemTrayIcon>      m_trayIcon;               /** tray icon object.                            */
		std::unique_ptr<CaptureDesktopThread> m_captureThread;          /** capture and composition thread.              */
		QMutex                                m_mutex;                  /** mutex for data integrity.                    */
		QTimer                                m_timer;                  /** timer.                                       */
		unsigned long                         m_secuentialNumber;       /** frame number.                                */
		bool                                  m_started;                /** true if capturing, false otherwise.          */
		PomodoroStatistics                   *m_statisticsDialog;       /** Pomodoro statistics dialog object.           */
		std::shared_ptr<VPX_Interface>        m_vp8_interface;          /** VPX codec interface.                         */
		float                                 m_scale;                  /** output scale ratio.                          */
		bool                                  m_paused;                 /** true if pomodoro is paused, false otherwise. */

		QAction *m_menuPause;         /** tray menu pause.                    */
		QAction *m_menuShowStats;     /** tray menu show pomodoro statistics. */
		QAction *m_menuShow;          /** tray menu application show.         */
		QAction *m_menuStopCapture;   /** tray menu stop capture.             */
		QAction *m_menuChangeTask;    /** tray menu change pomodoro task.     */
		QAction *m_menuAbout;         /** tray menu about application.        */
		QAction *m_menuQuit;          /** tray menu quit application.         */

		Configuration m_config; /** application configuration struct. */
};

#endif // DESKTOP_CAPTURE_H_
