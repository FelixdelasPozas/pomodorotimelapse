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
#include <QMutexLocker>
#include <QDebug>

const QList<QPainter::CompositionMode> CaptureDesktopThread::COMPOSITION_MODES = { QPainter::CompositionMode_SourceOver,
		                                                                               QPainter::CompositionMode_Plus,
		                                                                               QPainter::CompositionMode_Multiply };
const QStringList CaptureDesktopThread::COMPOSITION_MODES_NAMES = { QString("Copy"),
		                                                                QString("Plus"),
		                                                                QString("Multiply") };
const QStringList CaptureDesktopThread::POSITION_NAMES = { QString("Free"),
																													 QString("Top Left"),
																													 QString("Top Center"),
																													 QString("Top Right"),
																													 QString("Center Left"),
																													 QString("Center"),
																													 QString("Center Right"),
																													 QString("Bottom Left"),
																													 QString("Bottom Center"),
																													 QString("Bottom Right") };

//-----------------------------------------------------------------
CaptureDesktopThread::CaptureDesktopThread(int capturedMonitor,
		                                       Resolution cameraResolution,
		                                       QPoint overlayPosition,
		                                       QPainter::CompositionMode compositionMode,
		                                       QObject* parent)
: QThread(parent)
, m_aborted{false}
, m_cameraEnabled{true}
, m_paused{false}
, m_compositionMode{compositionMode}
, m_paintFrame{false}
{
	setCaptureMonitor(capturedMonitor);
	setOverlayPosition(overlayPosition);

	if (cameraResolution.name != QString())
	{
		setResolution(cameraResolution);
		setCameraEnabled(true);
	}
	else
		m_cameraEnabled = false;
}

//-----------------------------------------------------------------
CaptureDesktopThread::~CaptureDesktopThread()
{
	if (m_camera.isOpened())
		m_camera.release();
}

//-----------------------------------------------------------------
bool CaptureDesktopThread::setResolution(const Resolution &resolution)
{
	QMutexLocker lock(&m_mutex);

	bool opened = m_camera.isOpened();
	bool result;

	if (opened)
	{
		result  = m_camera.set(CV_CAP_PROP_FRAME_WIDTH, resolution.width);
		result &= m_camera.set(CV_CAP_PROP_FRAME_HEIGHT, resolution.height);
	}
	m_cameraResolution = resolution;

	return result;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCaptureMonitor(int monitor)
{
	QRect desktopGeometry;
	if (monitor == -1)
		desktopGeometry = QApplication::desktop()->geometry();
	else
		desktopGeometry = QApplication::desktop()->screenGeometry(monitor);

	m_mutex.lock();
	m_x = desktopGeometry.x();
	m_y = desktopGeometry.y();
	m_width = desktopGeometry.width();
	m_height = desktopGeometry.height();
	m_mutex.unlock();
}

//-----------------------------------------------------------------
bool CaptureDesktopThread::setCameraEnabled(bool enabled)
{
	QMutexLocker lock(&m_mutex);

	bool result = true;

	m_cameraEnabled = enabled;

	switch(m_cameraEnabled)
	{
		case true:
			if (!m_camera.isOpened())
			{
				result  = m_camera.open(0);
				result &= m_camera.set(CV_CAP_PROP_FRAME_WIDTH, m_cameraResolution.width);
				result &= m_camera.set(CV_CAP_PROP_FRAME_HEIGHT, m_cameraResolution.height);
			}
			break;
		case false:
			if (m_camera.isOpened())
				m_camera.release();
			result = true;
			break;
		default:
			break;
	}
	return result;
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
	m_paused = true;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::resume()
{
	if (!m_paused)
		return;

	m_mutex.lock();
	m_paused = false;
	m_mutex.unlock();

	m_pauseWaitCondition.wakeAll();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setOverlayPosition(const QPoint &point)
{
	QMutexLocker lock(&m_mutex);

	m_position = point;

	if (m_position.x() < 0)
		m_position.setX(0);

	if (m_position.y() < 0)
		m_position.setY(0);

  int xLimit = m_width - m_cameraResolution.width;
	if (m_position.x() > xLimit)
		m_position.setX(xLimit);


  int yLimit = m_height - m_cameraResolution.height;
	if (m_position.y() > yLimit)
		m_position.setY(yLimit);
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setOverlayPosition(QString positionName)
{
	int position = POSITION_NAMES.indexOf(positionName);
	if (position == -1)
		return;

	QMutexLocker lock(&m_mutex);

	switch (position)
	{
		case 1:
			m_position.setX(0);
			m_position.setY(0);
			break;
		case 2:
			m_position.setX(m_width/2 - m_cameraResolution.width/2);
			m_position.setY(0);
			break;
		case 3:
			m_position.setX(m_width - m_cameraResolution.width);
			m_position.setY(0);
			break;
		case 4:
			m_position.setX(0);
			m_position.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case 5:
			m_position.setX(m_width/2 - m_cameraResolution.width/2);
			m_position.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case 6:
			m_position.setX(m_width - m_cameraResolution.width);
			m_position.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case 7:
			m_position.setX(0);
			m_position.setY(m_height - m_cameraResolution.height);
			break;
		case 8:
			m_position.setX(m_width/2 - m_cameraResolution.width/2);
			m_position.setY(m_height - m_cameraResolution.height);
			break;
		case 9:
			m_position.setX(m_width - m_cameraResolution.width);
			m_position.setY(m_height - m_cameraResolution.height);
			break;
		case 0: // Free, nothing to be done
		default:
			break;
	}
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

		takeScreenshot();

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
        qDebug() << "ERROR: Mat could not be converted to QImage. Mat type is" << mat.type();
        return QImage();
    }
}

//-----------------------------------------------------------------
void CaptureDesktopThread::overlayCameraImage(QImage &baseImage, const QImage &overlayImage)
{
  QPainter painter(&baseImage);
  painter.setCompositionMode(m_compositionMode);
  painter.drawImage(m_position.x(), m_position.y(), overlayImage);

  if (m_paintFrame)
  {
  	QPoint middle(m_cameraResolution.width/2, m_cameraResolution.height/2);
  	QPolygon poly(5);
    painter.setPen(QColor(Qt::blue));

  	for (int i = 0; i < 3; ++i)
  	{
    	poly.setPoint(0, m_position.x()+i, m_position.y()+i);
    	poly.setPoint(1, m_position.x()+m_cameraResolution.width-i, m_position.y()+i);
    	poly.setPoint(2, m_position.x()+m_cameraResolution.width-i, m_position.y()+m_cameraResolution.height-i);
    	poly.setPoint(3, m_position.x()+i, m_position.y()+m_cameraResolution.height-i);
    	poly.setPoint(4, m_position.x()+i, m_position.y()+i);
    	painter.drawConvexPolygon(poly);
  		painter.drawLine(QPoint(m_position.x() + middle.x() - 50, m_position.y() + middle.y() - 1+i), QPoint(m_position.x() + middle.x() + 50, m_position.y() + middle.y() - 1+i));
    	painter.drawLine(QPoint(m_position.x() + middle.x() - 1+i, m_position.y() + middle.y() - 50), QPoint(m_position.x() + middle.x() - 1+i, m_position.y() + middle.y() +50));
  	}
  }
  painter.end();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setOverlayCompositionMode(const QPainter::CompositionMode mode)
{
	m_compositionMode = mode;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setPaintFrame(bool status)
{
	m_paintFrame = status;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::takeScreenshot()
{
	// capture desktop
	QPixmap desktopImage = QPixmap::grabWindow(QApplication::desktop()->winId(), m_x, m_y, m_width, m_height);

	m_mutex.lock();
	// capture camera & composite
	if (m_cameraEnabled && m_camera.isOpened())
	{
		while (!m_camera.read(m_frame))
			usleep(100);

		m_mutex.unlock();
		QImage image = desktopImage.toImage();
		QImage cameraImage = MatToQImage(m_frame);
		overlayCameraImage(image, cameraImage);
		desktopImage = QPixmap::fromImage(image);
	}
	else
		m_mutex.unlock();

	m_mutex.lock();
	m_image = desktopImage;
	m_mutex.unlock();
}
