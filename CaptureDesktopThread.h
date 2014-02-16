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
#include <QWaitCondition>

class CaptureDesktopThread
: public QThread
{
	Q_OBJECT
	public:
		explicit CaptureDesktopThread(int capturedMonitor,
				                          Resolution cameraResolution,
				                          QObject *parent = nullptr);
		virtual ~CaptureDesktopThread();

		void abort()
		{ m_aborted = true; }

		void pause();
		void resume();

		void setCaptureMonitor(int monitor);
		void setCameraEnabled(bool);
		void setResolution(Resolution);

		void run();

		QImage *getImage();

	signals:
		void render();

	private:
		bool             m_aborted;
		QImage           m_image;
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
};

#endif // CAPTURE_DESKTOP_THREAD_H_
