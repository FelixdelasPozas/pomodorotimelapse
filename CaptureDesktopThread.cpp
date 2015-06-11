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

// dLib
#include <dlib/opencv.h>
#include <dlib/gui_widgets.h>

const QList<QPainter::CompositionMode> CaptureDesktopThread::COMPOSITION_MODES_QT = { QPainter::CompositionMode_SourceOver,
		                                                                                  QPainter::CompositionMode_Plus,
		                                                                                  QPainter::CompositionMode_Multiply };

const QList<struct CaptureDesktopThread::Mask> CaptureDesktopThread::MASKS = { { QString(":/DesktopCapture/monocle.png"),   200, 285, QPoint(150,107) },
                                                                               { QString(":/DesktopCapture/guyfawkes.png"), 275, 285, QPoint(167,292) } };

//-----------------------------------------------------------------
CaptureDesktopThread::CaptureDesktopThread(int monitor,
		                                       Resolution cameraResolution,
		                                       COMPOSITION_MODE compositionMode,
		                                       QPoint cameraOverlayPosition,
		                                       QPoint statsOverlayPosition,
		                                       QObject* parent)
: QThread          {parent}
, m_aborted        {false}
, m_paused         {false}
, m_cameraEnabled  {true}
, m_compositionMode{compositionMode}
, m_paintFrame     {false}
, m_pomodoro       {nullptr}
, m_isTracking     {false}
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

	m_faceDetector = dlib::get_frontal_face_detector();
	dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> m_faceShape;
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
	m_isTracking = false;

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
	QMutexLocker lock(&m_mutex);

	if (monitor == -1)
	{
		m_geometry = QApplication::desktop()->geometry();
	}
	else
	{
		m_geometry = QApplication::desktop()->screenGeometry(monitor);
	}
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

  int xLimit = m_geometry.width() - m_cameraResolution.width;
  int yLimit = m_geometry.height() - m_cameraResolution.height;

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

	auto width = m_geometry.width();
	auto height = m_geometry.height();

	switch (position)
	{
		case POSITION::TOP_LEFT:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(0);
			break;
		case POSITION::TOP_CENTER:
			m_cameraPosition.setX(width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(0);
			break;
		case POSITION::TOP_RIGHT:
			m_cameraPosition.setX(width - m_cameraResolution.width);
			m_cameraPosition.setY(0);
			break;
		case POSITION::CENTER_LEFT:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(height/2-m_cameraResolution.height/2);
			break;
		case POSITION::CENTER:
			m_cameraPosition.setX(width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(height/2-m_cameraResolution.height/2);
			break;
		case POSITION::CENTER_RIGHT:
			m_cameraPosition.setX(width - m_cameraResolution.width);
			m_cameraPosition.setY(height/2-m_cameraResolution.height/2);
			break;
		case POSITION::BOTTOM_LEFT:
			m_cameraPosition.setX(0);
			m_cameraPosition.setY(height - m_cameraResolution.height);
			break;
		case POSITION::BOTTOM_CENTER:
			m_cameraPosition.setX(width/2 - m_cameraResolution.width/2);
			m_cameraPosition.setY(height - m_cameraResolution.height);
			break;
		case POSITION::BOTTOM_RIGHT:
			m_cameraPosition.setX(width - m_cameraResolution.width);
			m_cameraPosition.setY(height - m_cameraResolution.height);
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
			m_isTracking = false;
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

  int xLimit = m_geometry.width() - 250;
  int yLimit = m_geometry.height() - pomodoroOverlayHeight();

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
void CaptureDesktopThread::overlayCameraImage(QImage &baseImage, QImage &overlayImage, QList<dlib::full_object_detection> shapes)
{
  QPainter painter(&baseImage);

  QPainter facePainter(&overlayImage);
  QPen pen;
  pen.setWidth(2);
  pen.setColor(Qt::white);
  facePainter.setPen(pen);

  QImage cropped;
  if(m_isTracking)
  {
    for(auto shape: shapes)
    {
      // left eye coordinate
      int lx = 0, ly = 0;
      for(int i = 36; i <= 41; ++i)
      {
        lx += shape.part(i).x();
        ly += shape.part(i).y();
      }
      lx /= 6;
      ly /= 6;

      // right eye coordinate
      int rx = 0, ry = 0;
      for(int i = 42; i <= 47; ++i)
      {
        rx += shape.part(i).x();
        ry += shape.part(i).y();
      }
      rx /= 6;
      ry /= 6;

      // mouth coordinate
      int mx = shape.part(51).x();
      int my = shape.part(51).y();

      // Transform and paint the monocle.
      QImage mask;
      mask.load(MASKS[0].resource);

      QLineF line(QPoint(lx,ly),QPoint(rx,ry));
      line.angle();

      auto eyeDistance = std::sqrt(std::pow(rx-lx,2)+std::pow(ry-ly, 2));
      auto lipDistance = std::sqrt(std::pow(((lx+rx)/2) - mx, 2) + std::pow(((ly+ry)/2) - my, 2));
      auto widthRatio = eyeDistance/MASKS[0].eyeDistance;
      auto heightRatio = lipDistance/MASKS[0].lipDistance;

      QTransform transform;
      transform.scale(widthRatio, heightRatio);
      mask = mask.transformed(transform, Qt::SmoothTransformation);
      auto rotated = transform.map(MASKS[0].leftEye);

      facePainter.translate(QPoint(lx,ly));
      facePainter.rotate(-line.angle());
      facePainter.translate(-rotated.x(), -rotated.y());
      facePainter.drawImage(QPoint(0,0),mask);
    }

    auto rectangle = m_faceTracker.get_position();
    QPoint center((rectangle.left()+rectangle.right()) / 2, (rectangle.top()+rectangle.bottom()) /2);

    auto width = rectangle.width() * 1.6;
    auto height = rectangle.height() * 1.6;
    auto widthRatio = width / static_cast<double>(overlayImage.width());
    auto heightRatio = height / static_cast<double>(overlayImage.height());

    QRect rect;
    if(widthRatio > heightRatio)
    {
      rect = QRect(center.x() - (overlayImage.width()/2 * widthRatio), center.y() - (overlayImage.height()/2 * widthRatio), overlayImage.width() * widthRatio, overlayImage.height() * widthRatio);
    }
    else
    {
      rect = QRect(center.x() - (overlayImage.width()/2 * heightRatio), center.y() - (overlayImage.height()/2 * heightRatio), overlayImage.width() * heightRatio, overlayImage.height() * heightRatio);
    }

    cropped = overlayImage.copy(rect);
    cropped = cropped.scaled(overlayImage.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }

  painter.setCompositionMode(COMPOSITION_MODES_QT.at(static_cast<int>(m_compositionMode)));
  if(m_isTracking)
  {
    painter.drawImage(m_cameraPosition.x(), m_cameraPosition.y(), cropped);
  }
  else
  {
    painter.drawImage(m_cameraPosition.x(), m_cameraPosition.y(), overlayImage);
  }

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
	m_mutex.lock();

	static int frames = 0;

	// capture desktop
	auto desktopImage = QPixmap::grabWindow(QApplication::desktop()->winId(), m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height());

	// capture camera & composite
	if (m_cameraEnabled && m_camera.isOpened())
	{
		while (!m_camera.read(m_frame))
		{
			usleep(100);
		}
		m_mutex.unlock();

		dlib::cv_image<dlib::bgr_pixel> cimg(m_frame);
		QList<dlib::full_object_detection> shapes;

    auto faces = m_faceDetector(cimg);

    if(!faces.empty())
    {
      auto trackFace = faces.front();
      auto trackArea = trackFace.width() * trackFace.height();
      for(auto face: faces)
      {
        auto area = face.width() * face.height();
        if(area > trackArea)
        {
          trackFace = face;
          trackArea = area;
        }
      }

      if(!m_isTracking)
      {
        m_faceTracker.start_track(cimg, trackFace);
        m_isTracking = true;
      }
      else
      {
        m_faceTracker.update(cimg);
      }

      shapes << m_faceShape(cimg, trackFace);
    }

		++frames;
		auto image = desktopImage.toImage();
		auto cameraImage = MatToQImage(m_frame);
		overlayCameraImage(image, cameraImage, shapes);
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
