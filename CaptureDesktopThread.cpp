/*
    File: CaptureDesktopThread.cpp
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

// Project
#include <CaptureDesktopThread.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QImage>
#include <QPainter>
#include <QMutexLocker>
#include <QDebug>

const QList<QPainter::CompositionMode> CaptureDesktopThread::COMPOSITION_MODES_QT = { QPainter::CompositionMode_SourceOver,
		                                                                                  QPainter::CompositionMode_Plus,
		                                                                                  QPainter::CompositionMode_Multiply };

//-----------------------------------------------------------------
CaptureDesktopThread::CaptureDesktopThread(int monitor,
		                                       Resolution cameraResolution,
		                                       COMPOSITION_MODE compositionMode,
		                                       QPoint cameraOverlayPosition,
		                                       QPoint statsOverlayPosition,
		                                       QObject* parent)
: QThread          {parent}
, m_aborted        {false}
, m_cameraEnabled  {true}
, m_paused         {false}
, m_compositionMode{compositionMode}
, m_paintFrame     {false}
, m_pomodoro       {nullptr}
{
	setMonitor(monitor);
	setCameraOverlayPosition(cameraOverlayPosition);
	setStatsOverlayPosition(statsOverlayPosition);

	if (cameraResolution.name != QString())
	{
		setResolution(cameraResolution);
		setCameraEnabled(true);
	}
	else
	{
		m_cameraEnabled = false;
	}
}

//-----------------------------------------------------------------
CaptureDesktopThread::~CaptureDesktopThread()
{
	if (m_camera.isOpened())
	{
		m_camera.release();
	}
}

//-----------------------------------------------------------------
bool CaptureDesktopThread::setResolution(const Resolution &resolution)
{
	bool result = false;

	QMutexLocker lock(&m_mutex);

	m_cameraResolution = resolution;

	if (m_camera.isOpened())
	{
		result  = m_camera.set(CV_CAP_PROP_FRAME_WIDTH, resolution.width);
		result &= m_camera.set(CV_CAP_PROP_FRAME_HEIGHT, resolution.height);
	}

	return result;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setMonitor(int monitor)
{
	QRect desktopGeometry;

	if (monitor == -1)
	{
		desktopGeometry = QApplication::desktop()->geometry();
	}
	else
	{
		desktopGeometry = QApplication::desktop()->screenGeometry(monitor);
	}

	QMutexLocker lock(&m_mutex);

	m_x      = desktopGeometry.x();
	m_y      = desktopGeometry.y();
	m_width  = desktopGeometry.width();
	m_height = desktopGeometry.height();
}

//-----------------------------------------------------------------
bool CaptureDesktopThread::setCameraEnabled(bool enabled)
{
	bool result = true;

	QMutexLocker lock(&m_mutex);

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
			{
				m_camera.release();
			}
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
	if (m_paused) return;

	QMutexLocker lock(&m_mutex);
	m_paused = true;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::resume()
{
	if (!m_paused) return;

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
	{
		m_cameraPosition.setX(0);
	}

	if (m_cameraPosition.y() < 0)
	{
		m_cameraPosition.setY(0);
	}

  int xLimit = m_width - m_cameraResolution.width;
  int yLimit = m_height - m_cameraResolution.height;

	if (m_cameraPosition.x() > xLimit)
	{
		m_cameraPosition.setX(xLimit);
	}

	if (m_cameraPosition.y() > yLimit)
	{
		m_cameraPosition.setY(yLimit);
	}
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCameraOverlayPosition(POSITION position)
{
	QMutexLocker lock(&m_mutex);

	switch (position)
	{
		case POSITION::TOP_LEFT:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(0);
			break;
		case POSITION::TOP_CENTER:
			m_cameraPosition.setX(m_width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(0);
			break;
		case POSITION::TOP_RIGHT:
			m_cameraPosition.setX(m_width - m_cameraResolution.width);
			m_cameraPosition.setY(0);
			break;
		case POSITION::CENTER_LEFT:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case POSITION::CENTER:
			m_cameraPosition.setX(m_width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case POSITION::CENTER_RIGHT:
			m_cameraPosition.setX(m_width - m_cameraResolution.width);
			m_cameraPosition.setY(m_height/2-m_cameraResolution.height/2);
			break;
		case POSITION::BOTTOM_LEFT:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(m_height - m_cameraResolution.height);
			break;
		case POSITION::BOTTOM_CENTER:
			m_cameraPosition.setX(m_width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(m_height - m_cameraResolution.height);
			break;
		case POSITION::BOTTOM_RIGHT:
			m_cameraPosition.setX(m_width - m_cameraResolution.width);
			m_cameraPosition.setY(m_height - m_cameraResolution.height);
			break;
		case POSITION::FREE: // nothing to be done.
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
		{
			m_pauseWaitCondition.wait(&m_mutex);
		}
		m_mutex.unlock();

		takeScreenshot();

		emit imageAvailable();
	}
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setPomodoro(std::shared_ptr<Pomodoro> pomodoro)
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
	{
		m_statsPosition.setX(0);
	}

	if (m_statsPosition.y() < 0)
	{
		m_statsPosition.setY(0);
	}

  int xLimit = m_width - 250;
  int yLimit = m_height - pomodoroOverlayHeight();

	if (m_statsPosition.x() > xLimit)
	{
		m_statsPosition.setX(xLimit);
	}

	if (m_statsPosition.y() > yLimit)
	{
		m_statsPosition.setY(yLimit);
	}
}

//-----------------------------------------------------------------
QImage CaptureDesktopThread::MatToQImage(const cv::Mat& mat)
{
  if (mat.type() == CV_8UC1) // 8-bits unsigned, NO. OF CHANNELS=1
  {
    // Set the color table (used to translate color indexes to qRgb values)
    QVector<QRgb> colorTable;
    for (int i = 0; i < 256; i++)
    {
      colorTable.push_back(qRgb(i, i, i));
    }

    // Copy input Mat
    const uchar *qImageBuffer = (const uchar*) mat.data;

    // Create QImage with same dimensions as input Mat
    QImage img(qImageBuffer, mat.cols, mat.rows, mat.step, QImage::Format_Indexed8);
    img.setColorTable(colorTable);

    return img;
  }
  else // 8-bits unsigned, NO. OF CHANNELS=3
  {
    if (mat.type() == CV_8UC3)
    {
      // Copy input Mat
      const uchar *qImageBuffer = (const uchar*) mat.data;

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
}

//-----------------------------------------------------------------
void CaptureDesktopThread::overlayCameraImage(QImage &baseImage, const QImage &overlayImage)
{
  QPainter painter(&baseImage);
  painter.setCompositionMode(COMPOSITION_MODES_QT.at(static_cast<int>(m_compositionMode)));
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
void CaptureDesktopThread::drawPomodoroUnit(QPainter &painter, QColor color, const QPoint &position, const QString &text, int width)
{
	QBrush brush(color);
	color.setAlphaF(0.8);
	painter.setBrush(brush);
	painter.fillRect(position.x(), position.y(), width, POMODORO_UNIT_HEIGHT, color);
	painter.setPen(Qt::white);
	painter.setOpacity(POMODORO_UNIT_OPACITY);
	painter.drawText(position.x() + POMODORO_UNIT_MARGIN, position.y() + POMODORO_UNIT_HEIGHT - POMODORO_UNIT_MARGIN, text);
}

//-----------------------------------------------------------------
int CaptureDesktopThread::pomodoroOverlayHeight()
{
  if (nullptr == m_pomodoro) return 0;

  return POMODORO_UNIT_HEIGHT * ((2 * m_pomodoro->getPomodorosInSession()) - 1 +
                                 (m_pomodoro->getPomodorosInSession() / m_pomodoro->getPomodorosBeforeLongBreak()) -
                                 ((m_pomodoro->getPomodorosInSession() % m_pomodoro->getPomodorosBeforeLongBreak() == 0) ? 1 : 0));
}
//-----------------------------------------------------------------
void CaptureDesktopThread::overlayPomodoro(QImage &image)
{
	if (m_pomodoro == nullptr) return;

	static unsigned long total = 0;

	QPainter painter(&image);
	QFont font = painter.font();
	font.setBold(true);
	int height = pomodoroOverlayHeight();
	QColor color = Qt::lightGray;
	color.setAlphaF(0.33);
	painter.fillRect(m_statsPosition.x(), m_statsPosition.y(), POMODORO_UNIT_MAX_WIDTH, height, color);

	if (m_paintFrame)
	{
	 	QPolygon poly(5);
    painter.setPen(QColor(Qt::yellow));
  	for (int i = 0; i < 3; ++i)
  	{
    	poly.setPoint(0, m_statsPosition.x()+i, m_statsPosition.y()+i);
    	poly.setPoint(1, m_statsPosition.x()+ POMODORO_UNIT_MAX_WIDTH-i, m_statsPosition.y()+i);
    	poly.setPoint(2, m_statsPosition.x()+ POMODORO_UNIT_MAX_WIDTH-i, m_statsPosition.y()+ height-i);
    	poly.setPoint(3, m_statsPosition.x()+i, m_statsPosition.y()+ height-i);
    	poly.setPoint(4, m_statsPosition.x()+i, m_statsPosition.y()+i);
    	painter.drawConvexPolygon(poly);
  	}

  	painter.setPen(Qt::white);
  	QFont serifFont("Times", 15, QFont::Bold);
  	painter.setFont(serifFont);
  	QFontMetrics metrics(serifFont);
  	int fontWidth = metrics.width(QString("Pomodoro"));
  	int fontHeight = metrics.height();
  	QPoint position = QPoint(m_statsPosition.x() + POMODORO_UNIT_MAX_WIDTH/2 - fontWidth/2 , (2*m_statsPosition.y()+height)/2 - fontHeight/2);
  	painter.drawText(position, QString("Pomodoro"));
  	fontWidth = metrics.width(QString("Statistics"));
  	position = QPoint(m_statsPosition.x() + POMODORO_UNIT_MAX_WIDTH/2 - fontWidth/2, position.y()+5 + fontHeight);
  	painter.drawText(position, QString("Statistics"));

	}
	else
	{
		QPoint position = m_statsPosition;
		for(unsigned int i = 1; i <= m_pomodoro->completedPomodoros(); ++i)
		{
			drawPomodoroUnit(painter, Qt::red, position, m_pomodoro->getCompletedTasks()[i-1]);
			position.setY(position.y() + POMODORO_UNIT_HEIGHT);

			if (i <= m_pomodoro->completedShortBreaks())
			{
				drawPomodoroUnit(painter, Qt::blue, position, QString("Short break"));
				position.setY(position.y() + POMODORO_UNIT_HEIGHT);
			}

			bool longBreak = (i % m_pomodoro->getPomodorosBeforeLongBreak() == 0) &&
					             ((i / m_pomodoro->getPomodorosBeforeLongBreak()) <= m_pomodoro->completedLongBreaks());

			if (longBreak)
			{
				drawPomodoroUnit(painter, Qt::green, position, QString("Long Break"));
				position.setY(position.y() + POMODORO_UNIT_HEIGHT);
			}
		}

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
				total = zero.msecsTo(m_pomodoro->getPomodoroDuration());
				break;
			case Pomodoro::Status::ShortBreak:
				color = Qt::blue;
				text = QString("In A Short Break");
				total = zero.msecsTo(m_pomodoro->getShortBreakDuration());
				break;
			case Pomodoro::Status::LongBreak:
				color = Qt::green;
				text = QString("In A Long Break");
				total = zero.msecsTo(m_pomodoro->getLongBreakDuration());
				break;
			case Pomodoro::Status::Paused:
				color = Qt::gray;
				text = QString("Paused");
				break;
			default:
				Q_ASSERT(false);
				break;
		}

		int pixels = static_cast<double>(mSec) / static_cast<double>(total) * 250;
		drawPomodoroUnit(painter, color, position, text, pixels);
	}
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCameraOverlayCompositionMode(COMPOSITION_MODE mode)
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
		{
			usleep(100);
		}

		m_mutex.unlock();
		QImage image = desktopImage.toImage();
		QImage cameraImage = MatToQImage(m_frame);
		overlayCameraImage(image, cameraImage);
		overlayPomodoro(image);
		desktopImage = QPixmap::fromImage(image);
	}
	else
	{
	  m_mutex.unlock();
	}

	m_mutex.lock();
	m_image = desktopImage;
	m_mutex.unlock();
}
