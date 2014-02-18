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
  	static const QStringList COMPOSITION_MODES_NAMES;

		explicit CaptureDesktopThread(int capturedMonitor,
				                          Resolution cameraResolution,
				                          QPoint overlayPosition,
				                          QPainter::CompositionMode compositionMode,
				                          QObject *parent = nullptr);
		virtual ~CaptureDesktopThread();

		void abort()
		{ m_aborted = true; }

		void pause();
		void resume();

		void setCaptureMonitor(int monitor);
		bool setCameraEnabled(bool);
		bool setResolution(const Resolution &resolution);
		void setOverlayPosition(const QPoint &point);
		void setOverlayCompositionMode(const QPainter::CompositionMode mode);
		void setPaintFrame(bool);

		void run();

		QPixmap *getImage();

	signals:
		void render();

	private:
		void overlayCameraImage(QImage &baseImage, const QImage &overlayImage);
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
		QPoint           m_position;

		QPainter::CompositionMode m_compositionMode;
		bool m_paintFrame;
};

#endif // CAPTURE_DESKTOP_THREAD_H_
