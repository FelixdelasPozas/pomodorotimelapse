/*
 * CaptureDesktopThread.h
 *
 *  Created on: 15/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef CAPTURE_DESKTOP_THREAD_H_
#define CAPTURE_DESKTOP_THREAD_H_

// Application
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

class CaptureDesktopThread
: public QThread
{
	Q_OBJECT
	public:
  	static const QList<QPainter::CompositionMode> COMPOSITION_MODES;
  	static const QStringList POSITION_NAMES;
  	static const QStringList COMPOSITION_MODES_NAMES;

		explicit CaptureDesktopThread(int capturedMonitor,
				                          Resolution cameraResolution,
				                          QPoint cameraOverlayPosition,
				                          QPoint statsOverlayPosition,
				                          QPainter::CompositionMode compositionMode,
				                          QObject *parent = nullptr);
		virtual ~CaptureDesktopThread();

		void abort()
		{ m_aborted = true; }

		void pause();
		void resume();
		bool isPaused()
		{ return m_paused; }

		void setCaptureMonitor(int monitor);
		bool setCameraEnabled(bool);
		bool setResolution(const Resolution &resolution);
		void setCameraOverlayPosition(const QPoint &point);
		void setCameraOverlayPosition(QString positionName);
		void setCameraOverlayCompositionMode(const QPainter::CompositionMode mode);
		void setStatsOverlayPosition(const QPoint &point);
		void setPaintFrame(bool);
		void setPomodoro(Pomodoro *pomodoro);
		QPoint getCameraOverlayPosition()
		{ return m_cameraPosition; };

		QPoint getStatsOverlayPosition()
		{ return m_statsPosition; }

		void run();

		QPixmap *getImage();
		void takeScreenshot();

	signals:
		void render();

	private:
		void overlayCameraImage(QImage &baseImage, const QImage &overlayImage);
		void overlayPomodoro(QImage &baseImage);
		void drawPomodoroUnit(QPainter &painter, QColor color, const QPoint &position, const QString &text, int width = 250);
		QImage MatToQImage(const cv::Mat& mat);

		bool             m_aborted;
		QPixmap          m_image;
		int              m_x;
		int              m_y;
		int              m_width;
		int              m_height;
		Resolution       m_cameraResolution;
		cv::VideoCapture m_camera;
		bool             m_cameraEnabled;
		bool             m_paused;
		QMutex           m_mutex;
		QWaitCondition   m_pauseWaitCondition;
		cv::Mat          m_frame;
		QPoint           m_cameraPosition;
		QPoint           m_statsPosition;
		Pomodoro        *m_pomodoro;

		QPainter::CompositionMode m_compositionMode;
		bool m_paintFrame;
};

#endif // CAPTURE_DESKTOP_THREAD_H_
