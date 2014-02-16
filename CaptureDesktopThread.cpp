/*
 * CaptureDesktopThread.cpp
 *
 *  Created on: 15/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

// Application
#include <CaptureDesktopThread.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QImage>
#include <QDebug>

//-----------------------------------------------------------------
CaptureDesktopThread::CaptureDesktopThread(int capturedMonitor,
		                                       Resolution cameraResolution,
		                                       QObject* parent)
: QThread(parent)
, m_aborted{false}
, m_cameraEnabled{true}
, m_paused{false}
{
	m_cameraResolution.height = cameraResolution.height;
	m_cameraResolution.width = cameraResolution.width;
	m_cameraResolution.name = cameraResolution.name;
	setCaptureMonitor(capturedMonitor);

	if (cameraResolution.name != QString())
		setCameraEnabled(m_cameraEnabled);
}

//-----------------------------------------------------------------
CaptureDesktopThread::~CaptureDesktopThread()
{
	if (m_camera.isOpened())
		m_camera.release();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setResolution(Resolution resolution)
{
	m_camera.set(CV_CAP_PROP_FRAME_WIDTH, resolution.width);
	m_camera.set(CV_CAP_PROP_FRAME_HEIGHT, resolution.height);
	m_cameraResolution = resolution;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCaptureMonitor(int monitor)
{
	QRect desktopGeometry;
	if (monitor == -1)
		desktopGeometry = QApplication::desktop()->geometry();
	else
		desktopGeometry = QApplication::desktop()->screenGeometry(monitor);

	m_x = desktopGeometry.x();
	m_y = desktopGeometry.y();
	m_width = desktopGeometry.width();
	m_height = desktopGeometry.height();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCameraEnabled(bool enabled)
{
	switch(enabled)
	{
		case true:
			if (!m_camera.isOpened())
			{
				m_camera.open(0);
				setResolution(m_cameraResolution);
			}
			break;
		case false:
			if (m_camera.isOpened())
				m_camera.release();
			break;
		default:
			break;
	}

}

//-----------------------------------------------------------------
QPixmap* CaptureDesktopThread::getImage()
{
	return &m_image;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::pause()
{
	if (m_paused)
		return;

	m_mutex.lock();
	if ((this->m_cameraEnabled) && m_camera.isOpened())
		m_camera.release();

	m_paused = true;
	m_mutex.unlock();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::resume()
{
	if (!m_paused)
		return;

	m_mutex.lock();
	if ((this->m_cameraEnabled) && !m_camera.isOpened())
		m_camera.open(0);

	m_paused = false;
	m_mutex.unlock();
	m_pauseWaitCondition.wakeAll();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::run()
{
	setPriority(Priority::NormalPriority);


	while (!m_aborted)
	{
		m_mutex.lock();
		if (m_paused)
			m_pauseWaitCondition.wait(&m_mutex);
		m_mutex.unlock();

		// capture desktop
		m_image = QPixmap::grabWindow(QApplication::desktop()->winId(), m_x, m_y, m_width, m_height);

		// capture camera
		m_mutex.lock();
		if (m_camera.isOpened())
		{

		}
		m_mutex.unlock();

		// image composite
		// TODO

		emit render();
	}
}
