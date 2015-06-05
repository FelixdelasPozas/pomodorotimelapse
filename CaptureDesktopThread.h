/*
    File: CaptureDesktopThread.h
    Created on: 15/02/2014
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

#ifndef CAPTURE_DESKTOP_THREAD_H_
#define CAPTURE_DESKTOP_THREAD_H_

// Project
#include "Resolutions.h"
#include "Pomodoro.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// Qt
#include <QThread>
#include <QLabel>
#include <QMutex>
#include <QPainter>
#include <QWaitCondition>

// C++
#include <memory>

class CaptureDesktopThread
: public QThread
{
	Q_OBJECT
	public:
	  enum class COMPOSITION_MODE: char { COPY, PLUS, MULTIPLY };
	  enum class POSITION: char { FREE, TOP_LEFT, TOP_CENTER, TOP_RIGHT, CENTER_LEFT, CENTER, CENTER_RIGHT, BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT };

  	/** \brief CaptureDesktopThread class constructor.
  	 * \param[in] monitor monitor index according to Qt or -1 to capture all monitors.
  	 * \param[in] cameraResolution resolution of the picture captured by the camera.
  	 * \param[in] compositionMode composition mode for the camera picture.
  	 * \param[in] cameraOverlayPosition camera overlay top left point.
  	 * \param[in] pomodoroOverlayPosition pomodoro statistics overlay top left point.
  	 * \param[in] parent raw pointer of the parent of this object.
  	 *
  	 */
		explicit CaptureDesktopThread(int              monitor,
				                          Resolution       cameraResolution,
				                          COMPOSITION_MODE compositionMode,
				                          QPoint           cameraOverlayPosition,
				                          QPoint           pomodoroOverlayPosition,
				                          QObject         *parent = nullptr);

		/** \brief CaptureDesktopThread class virtual destructor.
     *
     */
		virtual ~CaptureDesktopThread();

    /** \brief Aborts the thread.
     *
     */
		void abort()
		{ m_aborted = true; }

    /** \brief Pauses the thread.
     *
     */
		void pause();

		/** \brief Resumes the thread.
     *
     */
		void resume();

		/** \brief Returns true if the thread is paused and false otherwise.
     *
     */
		bool isPaused()
		{ return m_paused; }

    /** \brief Sets the monitor to capture.
     * \param[in] monitor monitor index according to Qt or -1 to capture all monitors.
     *
     */
		void setMonitor(int monitor);

    /** \brief Enables/disables the camera.
     * \param[in] enabled boolean value.
     *
     */
		bool setCameraEnabled(bool enabled);

    /** \brief Sets the resolution of the camera picture.
     * \param[in] resolution struct Resolution reference.
     *
     */
		bool setResolution(const Resolution &resolution);

    /** \brief Sets the position where the camera picture will be placed in the captured desktop image.
     * \param[in] point top-left point of the area to put the picture.
     *
     */
		void setCameraOverlayPosition(const QPoint &point);

    /** \brief Sets the position where the camera picture will be placed in the captured desktop image.
     * \param[in]
     *
     */
		void setCameraOverlayPosition(const POSITION position);

    /** \brief Sets the composition mode of the camera picture.
     *
     */
		void setCameraOverlayCompositionMode(const COMPOSITION_MODE mode);

    /** \brief Sets the position where the pomodoro statistics will be placed in the captured desktop image.
     * \param[in] point top-left point of the area to put the picture.
     *
     */
		void setStatsOverlayPosition(const QPoint &point);

    /** \brief Enables/disables the frame that shows the position of the camera and pomodoro overlays.
     *
     */
		void setPaintFrame(bool value);

    /** \brief Sets the pomodoro object to get the statistics of the overlay.
     * \param[in] pomodoro raw pointer of the pomodoro object.
     *
     */
		void setPomodoro(std::shared_ptr<Pomodoro> pomodoro);

    /** \brief Returns the top-left point of the camera overlay area.
     *
     */
		QPoint getCameraOverlayPosition()
		{ return m_cameraPosition; };

    /** \brief Returns the top-left point of the pomodoro statistics overlay area.
     *
     */
		QPoint getStatsOverlayPosition()
		{ return m_statsPosition; }

		virtual void run() final;

    /** \brief Returns the final composed image.
     *
     */
		QPixmap *getImage();

    /** \brief Takes a picture of the desktop.
     *
     */
		void takeScreenshot();

		static constexpr int   POMODORO_UNIT_MAX_WIDTH = 250;
		static constexpr int   POMODORO_UNIT_HEIGHT    = 15;
		static constexpr int   POMODORO_UNIT_MARGIN    = 2;
		static constexpr float POMODORO_UNIT_OPACITY   = 0.8;

	signals:
		void imageAvailable();

	private:
		static const QList<QPainter::CompositionMode> COMPOSITION_MODES_QT;

    /** \brief Overlays the camera image over the desktop captured image.
     * \param[in/out] baseImage captured desktop image.
     * \param[in] overlayImage camera picture.
     *
     */
		void overlayCameraImage(QImage &baseImage, const QImage &overlayImage);

    /** \brief Overlays the pomodoro statistics over the desktop captured image.
     * \param[in/out] baseImage captured desktop image.
     *
     */
		void overlayPomodoro(QImage &baseImage);

    /** \brief Draws a single pomodoro unit.
     * \param[in/out] painter painter object reference.
     * \param[in] color color of the unit.
     * \param[in] position top-left point of the area of the unit.
     * \param[in] text text of the pomodoro unit.
     * \param[in] width width in pixels of the unit.
     *
     */
		void drawPomodoroUnit(QPainter &painter, QColor color, const QPoint &position, const QString &text, int width = POMODORO_UNIT_MAX_WIDTH);

		/** \brief Returns the height of the pomodoro overlay.
		 *
		 */
		int pomodoroOverlayHeight();

    /** \brief Converts a MAT image from OpenCV to a Qt QImage.
     * \param[in] mat reference to a OpenCV mat image.
     *
     */
		QImage MatToQImage(const cv::Mat& mat);

		bool             m_aborted;              /** true if the thread has been aborted.                          */
		bool             m_paused;               /** true to stop capturing.                                       */
		QMutex           m_mutex;                /** thread mutex                                                  */
		QWaitCondition   m_pauseWaitCondition;   /** thread pause condition                                        */

		QPixmap          m_image;                /** final image after composition.                                */
		QRect            m_geometry;             /** geometry of the capture area                                  */
		Resolution       m_cameraResolution;     /** camera resolution                                             */
		cv::VideoCapture m_camera;               /** opencv camera                                                 */
		cv::Mat          m_frame;                /** opencv frame.                                                 */
		bool             m_cameraEnabled;        /** true if camera capture is enabled.                            */
		QPoint           m_cameraPosition;       /** position of the camera overlay                                */
		QPoint           m_statsPosition;        /** position of the statistics overlay                            */
		COMPOSITION_MODE m_compositionMode;      /** composition mode for the camera overlay.                      */
		bool             m_paintFrame;           /** true to paint a frame of the overlayed camera and statistics. */

		std::shared_ptr<Pomodoro> m_pomodoro;
};

#endif // CAPTURE_DESKTOP_THREAD_H_
