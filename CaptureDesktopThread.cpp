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
#include <QPainter>
#include <QDebug>
#include <QMutexLocker>

//-----------------------------------------------------------------
CaptureDesktopThread::CaptureDesktopThread(int capturedMonitor,
		                                       Resolution cameraResolution,
		                                       QObject* parent)
: QThread(parent)
, m_aborted{false}
, m_cameraEnabled{true}
, m_paused{false}
{
	m_cameraResolution = cameraResolution;
	setCaptureMonitor(capturedMonitor);

	qDebug() << getResolutionAsString(m_cameraResolution);

	if (m_cameraResolution.name != QString())
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
	QMutexLocker lock(&m_mutex);
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
	QMutexLocker lock(&m_mutex);
	qDebug() << "llamada setEnabled con" << enabled << m_cameraResolution.width << m_cameraResolution.height;

	switch(enabled)
	{
		case true:
			if (!m_camera.isOpened())
			{
				m_camera.open(0);
				m_camera.set(CV_CAP_PROP_FRAME_WIDTH, m_cameraResolution.width);
				m_camera.set(CV_CAP_PROP_FRAME_HEIGHT, m_cameraResolution.height);
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
	QMutexLocker lock(&m_mutex);

	return &m_image;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::pause()
{
	if (m_paused)
		return;

	QMutexLocker lock(&m_mutex);
	if ((this->m_cameraEnabled) && m_camera.isOpened())
		m_camera.release();

	m_paused = true;
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
      if (m_camera.read(m_frame))
      {
    		QImage image = m_image.toImage();
      	QImage cameraImage = MatToQImage(m_frame);
      	overlayCameraImage(image, cameraImage, QPoint(0,0));

      	m_image = QPixmap::fromImage(image);
      }
		}
		m_mutex.unlock();

		emit render();
	}
}

//-----------------------------------------------------------------
QImage CaptureDesktopThread::MatToQImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS=1
    if(mat.type()==CV_8UC1)
    {
        // Set the color table (used to translate color indexes to qRgb values)
        QVector<QRgb> colorTable;
        for (int i=0; i<256; i++)
            colorTable.push_back(qRgb(i,i,i));

        // Copy input Mat
        const uchar *qImageBuffer = (const uchar*)mat.data;

        // Create QImage with same dimensions as input Mat
        QImage img(qImageBuffer, mat.cols, mat.rows, mat.step, QImage::Format_Indexed8);
        img.setColorTable(colorTable);
        return img;
    }

    // 8-bits unsigned, NO. OF CHANNELS=3
    else if(mat.type()==CV_8UC3)
    {
        // Copy input Mat
        const uchar *qImageBuffer = (const uchar*)mat.data;

        // Create QImage with same dimensions as input Mat
        QImage img(qImageBuffer, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return img.rgbSwapped();
    }
    else
    {
        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}

//-----------------------------------------------------------------
void CaptureDesktopThread::overlayCameraImage(QImage &baseImage, const QImage &overlayImage, const QPoint &position)
{
    QPainter painter(&baseImage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    int x = m_width - overlayImage.size().width();
    int y = m_height - overlayImage.size().height();
    painter.drawImage(x, y, overlayImage);
    painter.end();
}
