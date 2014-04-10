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
		                                       QPoint cameraOverlayPosition,
		                                       QPoint statsOverlayPosition,
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
	setCameraOverlayPosition(cameraOverlayPosition);
	setStatsOverlayPosition(statsOverlayPosition);

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
	bool result = false;

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
void CaptureDesktopThread::setCameraOverlayPosition(const QPoint &point)
{
	QMutexLocker lock(&m_mutex);

	m_cameraPosition = point;

	if (m_cameraPosition.x() < 0)
		m_cameraPosition.setX(0);

	if (m_cameraPosition.y() < 0)
		m_cameraPosition.setY(0);

  int xLimit = m_width - m_cameraResolution.width;
	if (m_cameraPosition.x() > xLimit)
		m_cameraPosition.setX(xLimit);


  int yLimit = m_height - m_cameraResolution.height;
	if (m_cameraPosition.y() > yLimit)
		m_cameraPosition.setY(yLimit);
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCameraOverlayPosition(QString positionName)
{
	int position = POSITION_NAMES.indexOf(positionName);
	if (position == -1)
		return;

	QMutexLocker lock(&m_mutex);

	switch (position)
	{
		case 1:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(0);
			break;
		case 2:
			m_cameraPosition.setX(m_width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(0);
			break;
		case 3:
			m_cameraPosition.setX(m_width - m_cameraResolution.width);
			m_cameraPosition.setY(0);
			break;
		case 4:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case 5:
			m_cameraPosition.setX(m_width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case 6:
			m_cameraPosition.setX(m_width - m_cameraResolution.width);
			m_cameraPosition.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case 7:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(m_height - m_cameraResolution.height);
			break;
		case 8:
			m_cameraPosition.setX(m_width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(m_height - m_cameraResolution.height);
			break;
		case 9:
			m_cameraPosition.setX(m_width - m_cameraResolution.width);
			m_cameraPosition.setY(m_height - m_cameraResolution.height);
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
void CaptureDesktopThread::setPomodoro(Pomodoro *pomodoro)
{
	QMutexLocker lock(&m_mutex);
	m_pomodoro = pomodoro;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setStatsOverlayPosition(const QPoint& point)
{
	QMutexLocker lock(&m_mutex);

	m_statsPosition = point;

	if (m_statsPosition.x() < 0)
		m_statsPosition.setX(0);

	if (m_statsPosition.y() < 0)
		m_statsPosition.setY(0);

  int xLimit = m_width - 50;
	if (m_statsPosition.x() > xLimit)
		m_statsPosition.setX(xLimit);


  int yLimit = m_height - (15 * m_pomodoro->getPomodorosInSession());
	if (m_statsPosition.y() > yLimit)
		m_statsPosition.setY(yLimit);
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
  painter.drawImage(m_cameraPosition.x(), m_cameraPosition.y(), overlayImage);

  if (m_paintFrame)
  {
  	QPoint middle(m_cameraResolution.width/2, m_cameraResolution.height/2);
    int minimum = ((middle.x() < middle.y()) ? middle.x()/4 : middle.y()/4);

  	QPolygon poly(5);
    painter.setPen(QColor(Qt::blue));

  	for (int i = 0; i < 3; ++i)
  	{
    	poly.setPoint(0, m_cameraPosition.x()+i, m_cameraPosition.y()+i);
    	poly.setPoint(1, m_cameraPosition.x()+m_cameraResolution.width-i, m_cameraPosition.y()+i);
    	poly.setPoint(2, m_cameraPosition.x()+m_cameraResolution.width-i, m_cameraPosition.y()+m_cameraResolution.height-i);
    	poly.setPoint(3, m_cameraPosition.x()+i, m_cameraPosition.y()+m_cameraResolution.height-i);
    	poly.setPoint(4, m_cameraPosition.x()+i, m_cameraPosition.y()+i);
    	painter.drawConvexPolygon(poly);
  		painter.drawLine(QPoint(m_cameraPosition.x() + middle.x() - minimum, m_cameraPosition.y() + middle.y() - 1+i), QPoint(m_cameraPosition.x() + middle.x() + minimum, m_cameraPosition.y() + middle.y() - 1+i));
    	painter.drawLine(QPoint(m_cameraPosition.x() + middle.x() - 1+i, m_cameraPosition.y() + middle.y() - minimum), QPoint(m_cameraPosition.x() + middle.x() - 1+i, m_cameraPosition.y() + middle.y() + minimum));
  	}
  }
  painter.end();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::drawPomodoroUnit(QPainter &painter, const QColor &color, const QPoint &position, const QString &text, int width)
{
	QBrush brush(color);
	painter.setBrush(brush);
	painter.fillRect(position.x(), position.y(), 50, width, color);
	painter.drawText(position.x()+5, position.y(), text);
}

//-----------------------------------------------------------------
void CaptureDesktopThread::overlayPomodoro(QImage &image)
{
	if (m_pomodoro == nullptr)
		return;

	static unsigned long total = 0;

	QPainter painter(&image);

	if (m_paintFrame)
	{
  	QPolygon poly(5);
    painter.setPen(QColor(Qt::yellow));
    int height = 15 * m_pomodoro->getPomodorosInSession();
  	for (int i = 0; i < 3; ++i)
  	{
    	poly.setPoint(0, m_statsPosition.x()+i, m_statsPosition.y()+i);
    	poly.setPoint(1, m_statsPosition.x()+ 50-i, m_statsPosition.y()+i);
    	poly.setPoint(2, m_statsPosition.x()+ 50-i, m_statsPosition.y()+ height-i);
    	poly.setPoint(3, m_statsPosition.x()+i, m_statsPosition.y()+ height-i);
    	poly.setPoint(4, m_statsPosition.x()+i, m_statsPosition.y()+i);
    	painter.drawConvexPolygon(poly);
  	}
	}
	else
	{
		QPoint position = m_statsPosition;
		for(unsigned int i = 0; i < m_pomodoro->completedPomodoros(); ++i)
		{
			drawPomodoroUnit(painter, Qt::red, position, m_pomodoro->getCompletedTasks()[i]);
			position.setY(position.y() + 15);

			if (m_pomodoro->completedShortBreaks() >= i)
			{
				drawPomodoroUnit(painter, Qt::blue, position, QString("Short break"));
				position.setY(position.y() + 15);
			}

			if (i != 0 && (i % m_pomodoro->getPomodorosBeforeBreak() == 0))
			{
				drawPomodoroUnit(painter, Qt::green, position, QString("Long Break"));
				position.setY(position.y()+15);
			}
		}

		QColor color;
		QString text;
		unsigned long mSec = m_pomodoro->elapsed();
		QTime zero;
		switch(m_pomodoro->status())
		{
			case Pomodoro::Status::Stopped:
				return;
				break;
			case Pomodoro::Status::Pomodoro:
				color = Qt::red;
				text = m_pomodoro->getTask();
				total = zero.msecsTo(m_pomodoro->getPomodoroTime());
				break;
			case Pomodoro::Status::ShortBreak:
				color = Qt::blue;
				text = QString("In A Short Break");
				total = zero.msecsTo(m_pomodoro->getShortBreakTime());
				break;
			case Pomodoro::Status::LongBreak:
				color = Qt::green;
				text = QString("In A Long Break");
				total = zero.msecsTo(m_pomodoro->getLongBreakTime());
				break;
			case Pomodoro::Status::Paused:
				color = Qt::gray;
				text = QString("Paused");
				break;
			default:
				Q_ASSERT(false);
				break;
		}
		int pixels = static_cast<double>(mSec) / static_cast<double>(total) * 15;
		drawPomodoroUnit(painter, color, position, text, pixels);
	}
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCameraOverlayCompositionMode(const QPainter::CompositionMode mode)
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
		overlayPomodoro(image);
		desktopImage = QPixmap::fromImage(image);
	}
	else
		m_mutex.unlock();

	m_mutex.lock();
	m_image = desktopImage;
	m_mutex.unlock();
}
