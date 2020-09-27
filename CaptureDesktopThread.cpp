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
#include <QFont>
#include <QFontMetrics>
#include <QRgb>
#include <QDebug>
#include <QScreen>

// dLib
#include <dlib/opencv.h>
#include <dlib/gui_widgets.h>


const QList<QPainter::CompositionMode> CaptureDesktopThread::COMPOSITION_MODES_QT = { QPainter::CompositionMode_SourceOver,
		                                                                                  QPainter::CompositionMode_Plus,
		                                                                                  QPainter::CompositionMode_Multiply };

const QList<const struct CaptureDesktopThread::Mask> CaptureDesktopThread::MASKS = { { QString("I feel like a sir"),      QString(":/DesktopCapture/masks/monocle.png"),      200, 285, QPoint(150,107) },
                                                                                     { QString("Anonymous"),              QString(":/DesktopCapture/masks/guyfawkes.png"),    275, 285, QPoint(167,292) },
                                                                                     { QString("With a bottle of rum"),   QString(":/DesktopCapture/masks/pirate.png"),       110,  80, QPoint(142,216) },
                                                                                     { QString("Everything is awesome!"), QString(":/DesktopCapture/masks/awesome.png"),      110,  80, QPoint( 94,152) },
                                                                                     { QString("U mad?"),                 QString(":/DesktopCapture/masks/problem.png"),      110,  80, QPoint(124,120) },
                                                                                     { QString("Awesome face"),           QString(":/DesktopCapture/masks/awesome-face.png"),  74,  65, QPoint( 80, 85) },
                                                                                     { QString("Deal with it"),           QString(":/DesktopCapture/masks/dealwithit.png"),   100, 109, QPoint(105, 28) } };

const QString CaptureDesktopThread::CHAR_RAMP_LONG = QString(" .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$");

const QString CaptureDesktopThread::CHAR_RAMP_SHORT = QString(" .:-=+*#%@");

//-----------------------------------------------------------------
CaptureDesktopThread::CaptureDesktopThread(int monitor, Resolution cameraResolution, QObject* parent)
: QThread          {parent}
, m_aborted        {false}
, m_paused         {false}
, m_cameraEnabled  {false}
, m_compositionMode{COMPOSITION_MODE::COPY}
, m_drawFrame      {false}
, m_mask           {MASK::NONE}
, m_trackFace      {false}
, m_statisticsMode {COMPOSITION_MODE::COPY}
, m_ASCII_Art      {false}
, m_pomodoro       {nullptr}
{
	setMonitor(monitor);

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
void CaptureDesktopThread::setResolution(const Resolution &resolution)
{
	QMutexLocker lock(&m_mutex);

	m_cameraResolution = resolution;

	if (m_camera.isOpened())
	{
		m_camera.set(cv::CAP_PROP_FRAME_WIDTH, resolution.width);
		m_camera.set(cv::CAP_PROP_FRAME_HEIGHT, resolution.height);
	}
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
void CaptureDesktopThread::setCameraEnabled(bool enabled)
{
	QMutexLocker lock(&m_mutex);

	m_cameraEnabled = enabled;

	if(m_cameraEnabled)
	{
	  if (!m_camera.isOpened())
		{
			m_camera.open(0);
			m_camera.set(cv::CAP_PROP_FRAME_WIDTH, m_cameraResolution.width);
			m_camera.set(cv::CAP_PROP_FRAME_HEIGHT, m_cameraResolution.height);
		}
	}
	else
	{
	  if (m_camera.isOpened())
		{
			m_camera.release();
		}
	}
}

//-----------------------------------------------------------------
QPixmap* CaptureDesktopThread::getImage()
{
  // NOTE: not protected by mutex because the caller must get the mutex using getMutex()
	return &m_image;
}

//-----------------------------------------------------------------
QMutex* CaptureDesktopThread::getMutex()
{
  return &m_mutex;
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

  int xLimit = m_geometry.width() - m_cameraResolution.width;
  int yLimit = m_geometry.height() - m_cameraResolution.height;

	if (m_cameraPosition.x() < 0)      m_cameraPosition.setX(0);
	if (m_cameraPosition.x() > xLimit) m_cameraPosition.setX(xLimit);
	if (m_cameraPosition.y() < 0)      m_cameraPosition.setY(0);
	if (m_cameraPosition.y() > yLimit) m_cameraPosition.setY(yLimit);
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCameraOverlayPosition(const POSITION position)
{
	QMutexLocker lock(&m_mutex);

	if(position != POSITION::FREE)
	{
	  m_cameraPosition = computePosition(position, QRect(0,0, m_cameraResolution.width, m_cameraResolution.height));
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

  int xLimit = m_geometry.width() - POMODORO_UNIT_MAX_WIDTH;
  int yLimit = m_geometry.height() - pomodoroOverlayHeight();

	if (m_statsPosition.x() < 0) m_statsPosition.setX(0);
	if (m_statsPosition.y() < 0) m_statsPosition.setY(0);

	if (m_statsPosition.x() > xLimit) m_statsPosition.setX(xLimit);
	if (m_statsPosition.y() > yLimit) m_statsPosition.setY(yLimit);
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setStatsOverlayPosition(const POSITION position)
{
  QMutexLocker lock(&m_mutex);

  if(position != POSITION::FREE)
  {
    m_statsPosition = computePosition(position, QRect(0,0,POMODORO_UNIT_MAX_WIDTH, pomodoroOverlayHeight()));
  }
}

//-----------------------------------------------------------------
QPoint CaptureDesktopThread::getCameraOverlayPosition() const
{
  return m_cameraPosition;
}

//-----------------------------------------------------------------
QPoint CaptureDesktopThread::getStatsOverlayPosition() const
{
  return m_statsPosition;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setMask(MASK mask)
{
  QMutexLocker lock(&m_mutex);

  m_mask = mask;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setTrackFace(bool enabled)
{
  QMutexLocker lock(&m_mutex);

  if(enabled != m_trackFace)
  {
    m_trackFace = enabled;
  }
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCameraAsASCII(bool enabled)
{
  QMutexLocker lock(&m_mutex);

  if(enabled != m_ASCII_Art)
  {
    m_ASCII_Art = enabled;
  }
}

//-----------------------------------------------------------------
void CaptureDesktopThread::drawMask(QImage& cameraImage, dlib::full_object_detection shapes)
{
  auto maskIndex = static_cast<int>(m_mask);

  // left eye coordinate
  int lx = 0, ly = 0;
  for (int i = 36; i <= 41; ++i)
  {
    lx += shapes.part(i).x();
    ly += shapes.part(i).y();
  }
  lx /= 6;
  ly /= 6;

  // right eye coordinate
  int rx = 0, ry = 0;
  for (int i = 42; i <= 47; ++i)
  {
    rx += shapes.part(i).x();
    ry += shapes.part(i).y();
  }
  rx /= 6;
  ry /= 6;

  // mouth coordinate
  int mx = shapes.part(51).x();
  int my = shapes.part(51).y();

  // Transform and paint the mask.
  QImage mask;
  mask.load(MASKS[maskIndex].resource);

  QLineF line(QPoint(lx, ly), QPoint(rx, ry));
  line.angle();

  auto eyeDistance = std::sqrt(std::pow(rx - lx, 2) + std::pow(ry - ly, 2));
  auto lipDistance = std::sqrt(std::pow(((lx + rx) / 2) - mx, 2) + std::pow(((ly + ry) / 2) - my, 2));
  auto widthRatio = eyeDistance / MASKS[maskIndex].eyeDistance;
  auto heightRatio = lipDistance / MASKS[maskIndex].lipDistance;

  QTransform transform;
  transform.scale(widthRatio, heightRatio);
  mask = mask.transformed(transform, Qt::SmoothTransformation);
  auto rotatedLeftEye = transform.map(MASKS[maskIndex].leftEye);

  QPainter painter(&cameraImage);
  painter.translate(QPoint(lx, ly));
  painter.rotate(-line.angle());
  painter.translate(-rotatedLeftEye.x(), -rotatedLeftEye.y());
  painter.drawImage(QPoint(0, 0), mask);
  painter.end();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::drawFrame(QImage &image)
{
  QPoint middle(m_cameraPosition.x() + m_cameraResolution.width/2, m_cameraPosition.y() + m_cameraResolution.height/2);
  int minimum = ((m_cameraResolution.width/2 < m_cameraResolution.height/2) ? m_cameraResolution.width/8 : m_cameraResolution.height/8);

  QPolygon poly(5);
  QPainter painter(&image);
  painter.setPen(QColor(Qt::blue));
  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

  for (int i = 0; i < 5; ++i)
  {
    poly.setPoint(0, m_cameraPosition.x() + i, m_cameraPosition.y() + i);
    poly.setPoint(1, m_cameraPosition.x() + m_cameraResolution.width-i, m_cameraPosition.y() + i);
    poly.setPoint(2, m_cameraPosition.x() + m_cameraResolution.width-i, m_cameraPosition.y() + m_cameraResolution.height-i);
    poly.setPoint(3, m_cameraPosition.x() + i, m_cameraPosition.y() + m_cameraResolution.height-i);
    poly.setPoint(4, m_cameraPosition.x() + i, m_cameraPosition.y() + i);
    painter.drawConvexPolygon(poly);
    painter.drawLine(QPoint(middle.x() - minimum, middle.y() - 1+i), QPoint(middle.x() + minimum, middle.y() - 1+i));
    painter.drawLine(QPoint(middle.x() - 1+i, middle.y() - minimum), QPoint(middle.x() - 1+i, middle.y() + minimum));
  }

  painter.end();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::centerFace(QImage& cameraImage, const dlib::drectangle &rectangle)
{
  auto width = rectangle.width() * 1.7;
  auto height = rectangle.height() * 1.7;
  auto widthRatio = width / static_cast<double>(cameraImage.width());
  auto heightRatio = height / static_cast<double>(cameraImage.height());
  auto higherRatio = std::max(widthRatio, heightRatio);
  auto center = QPoint((rectangle.left()+rectangle.right()) / 2, (rectangle.top()+rectangle.bottom()) /2);
  auto rect = QRect(center.x() - (cameraImage.width()/2 * higherRatio), center.y() - (cameraImage.height()/2 * higherRatio), cameraImage.width() * higherRatio, cameraImage.height() * higherRatio);
  auto originalSize = cameraImage.size();

  cameraImage = cameraImage.copy(rect);
  cameraImage = cameraImage.scaled(originalSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

//-----------------------------------------------------------------
QPoint CaptureDesktopThread::computePosition(const POSITION position, const QRect& area)
{
  QPoint point;
  auto width = m_geometry.width();
  auto height = m_geometry.height();

  switch (position)
  {
    case POSITION::TOP_LEFT:
      point.setX(0);
      point.setY(0);
      break;
    case POSITION::TOP_CENTER:
      point.setX(width/2 - area.width()/2);
      point.setY(0);
      break;
    case POSITION::TOP_RIGHT:
      point.setX(width - area.width());
      point.setY(0);
      break;
    case POSITION::CENTER_LEFT:
      point.setX(0);
      point.setY(height/2 - area.height()/2);
      break;
    case POSITION::CENTER:
      point.setX(width/2 - area.width()/2);
      point.setY(height/2- area.height()/2);
      break;
    case POSITION::CENTER_RIGHT:
      point.setX(width - area.width());
      point.setY(height/2 - area.height()/2);
      break;
    case POSITION::BOTTOM_LEFT:
      point.setX(0);
      point.setY(height - area.height());
      break;
    case POSITION::BOTTOM_CENTER:
      point.setX(width/2 - area.width()/2);
      point.setY(height - area.height());
      break;
    case POSITION::BOTTOM_RIGHT:
      point.setX(width - area.width());
      point.setY(height - area.height());
      break;
    case POSITION::FREE: // nothing to be done.
    default:
      break;
  }

  return point;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setStatisticsOverlayCompositionMode(const COMPOSITION_MODE mode)
{
  m_statisticsMode = mode;
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
void CaptureDesktopThread::overlayCameraImage(QImage &baseImage, QImage &overlayImage)
{
  dlib::full_object_detection shapes;

  if(m_mask != MASK::NONE || m_trackFace)
  {
    double luma = 0;
    long i = 0;

    for(auto buf = m_frame.datastart; buf < m_frame.dataend; buf += 3, ++i)
      luma += (buf[2]*0.299) + (buf[1]*0.587) + (buf[0]*0.144);

    luma /= i;

    cv::normalize(m_frame, m_frame, 255, 128 - static_cast<int>(luma), cv::NORM_MINMAX);

    dlib::cv_image<dlib::bgr_pixel> cimg(m_frame);
    auto faces = m_faceDetector(cimg);

    if(!faces.empty())
    {
      auto trackFace = faces.front();
      auto trackArea = trackFace.area();
      for(auto face: faces)
      {
        const auto area = face.area();
        if(area > trackArea)
        {
          trackFace = face;
          trackArea = area;
        }
      }

      shapes = m_faceShape(cimg, trackFace);

      if(shapes.num_parts() > 0)
      {
        if (m_mask != MASK::NONE)
        {
          drawMask(overlayImage, shapes);
        }

        if (m_trackFace)
        {
          centerFace(overlayImage, shapes.get_rect());
        }
      }
    }
  }

  if(m_ASCII_Art)
  {
    imageToASCII(overlayImage);
  }

  QPainter painter(&baseImage);
  painter.setCompositionMode(COMPOSITION_MODES_QT.at(static_cast<int>(m_compositionMode)));
  painter.drawImage(m_cameraPosition.x(), m_cameraPosition.y(), overlayImage);
  painter.end();

  if (m_drawFrame)
  {
    drawFrame(baseImage);
  }
}

//-----------------------------------------------------------------
void CaptureDesktopThread::drawPomodoroUnit(QPainter &painter, QColor color, const QPoint &position, const QString &text, int width)
{
	QBrush brush(color);
	color.setAlphaF(0.8);
	painter.setClipRect(position.x(), position.y(), width, POMODORO_UNIT_HEIGHT);
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

  return POMODORO_UNIT_HEIGHT * ((2 * m_pomodoro->getPomodorosInSession()) - 1);
}

//-----------------------------------------------------------------
void CaptureDesktopThread::imageToASCII(QImage &image)
{
  // change ramp if desired. short ramp gives better pictures in my opinion.
  auto ramp = CHAR_RAMP_LONG;
  auto length = CHAR_RAMP_LONG.length();

  auto intensityToLetter = [ramp, length](unsigned int value) { return QString(ramp.at((value * (length-1)) / 255)); };

  QPainter painter(&image);
  painter.setPen(Qt::white);

  QFont font;
  font.setBold(true);
  font.setFixedPitch(true);
  font.setPixelSize(8);

  QFontMetrics fm(font);
  int textWidth  = fm.width("A");
  int textHeight = fm.height();

  painter.setFont(font);

  for(int x = 0; x < image.width() / textWidth; ++x)
  {
    for(int y = 0; y < image.height() / textHeight; ++y)
    {
      auto charX = x * textWidth;
      auto charY = y * textHeight;

      unsigned int value = 0;
      for(int i = 0; i < textWidth; ++i)
      {
        for(int j = 0; j < textHeight; ++j)
        {
          auto rgb = image.pixel(charX + i, charY + j);
          value += qGray(rgb);
        }
      }

      value /= textWidth * textHeight;
      const auto background = image.pixel(charX, charY);
      painter.fillRect(charX, charY, textWidth, textHeight, Qt::black);
      painter.setPen(background);
      painter.drawText(charX, charY, intensityToLetter(value));
    }
  }

  painter.end();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::overlayPomodoro(QImage &image)
{
	if (m_pomodoro == nullptr) return;

	static unsigned long total = 0;
	auto compositionMode = COMPOSITION_MODES_QT.at(static_cast<int>(m_statisticsMode));

	QPainter painter(&image);
	painter.setCompositionMode(compositionMode);

	QFont font = painter.font();
	font.setBold(true);
	int height = pomodoroOverlayHeight();
	QColor color = Qt::lightGray;
	color.setAlphaF(0.33);
	painter.fillRect(m_statsPosition.x(), m_statsPosition.y(), POMODORO_UNIT_MAX_WIDTH, height, color);

	if (m_drawFrame)
	{
	 	QPolygon poly(5);
    painter.setPen(QColor(Qt::yellow));
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
  	for (int i = 0; i < 5; ++i)
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
		auto completedPomodoros = m_pomodoro->completedPomodoros();
		auto pomodorosBeforeBreak = m_pomodoro->getPomodorosBeforeLongBreak();

		for(unsigned int i = 1; i <= completedPomodoros; ++i)
		{
			drawPomodoroUnit(painter, Qt::red, position, m_pomodoro->getCompletedTasks()[i-1]);
			position.setY(position.y() + POMODORO_UNIT_HEIGHT);

			auto numLongBreaks = i / pomodorosBeforeBreak;

			if(i % pomodorosBeforeBreak == 0)
			{
			  if(numLongBreaks <= m_pomodoro->completedLongBreaks())
			  {
          drawPomodoroUnit(painter, Qt::green, position, QString("Long Break %1").arg(QString().number(static_cast<int>(numLongBreaks))));
          position.setY(position.y() + POMODORO_UNIT_HEIGHT);
			  }
			}
			else
			{
			  auto numShortBreaks = i - numLongBreaks;
        if (numShortBreaks <= m_pomodoro->completedShortBreaks())
        {
          drawPomodoroUnit(painter, Qt::blue, position, QString("Short break %1").arg(QString().number(static_cast<int>(numShortBreaks))));
          position.setY(position.y() + POMODORO_UNIT_HEIGHT);
        }
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

		int pixels = static_cast<double>(mSec) / static_cast<double>(total) * POMODORO_UNIT_MAX_WIDTH;
		drawPomodoroUnit(painter, color, position, text, pixels);
	}

	painter.end();
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setCameraOverlayCompositionMode(COMPOSITION_MODE mode)
{
	m_compositionMode = mode;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::setPaintFrame(bool status)
{
	m_drawFrame = status;
}

//-----------------------------------------------------------------
void CaptureDesktopThread::takeScreenshot()
{
	m_mutex.lock();

	// capture desktop
	auto desktopPixmap = QApplication::screens().first()->grabWindow(QApplication::desktop()->winId(), m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height());

	if(m_pomodoro || m_cameraEnabled)
	{
	  auto desktopImage  = desktopPixmap.toImage();

	  // capture camera & composite
	  if (m_cameraEnabled && m_camera.isOpened())
	  {
	    while (!m_camera.read(m_frame))
	    {
	      usleep(100);
	    }
	    m_mutex.unlock();

      auto cameraImage = MatToQImage(m_frame);

      overlayCameraImage(desktopImage, cameraImage);
	  }
	  else
	  {
	    m_mutex.unlock();
	  }

	  if(m_pomodoro)
	  {
	    overlayPomodoro(desktopImage);
	  }

	  desktopPixmap = QPixmap::fromImage(desktopImage);
	}
	else
	{
	  m_mutex.unlock();
	}

	m_mutex.lock();
	m_image = desktopPixmap;
	m_mutex.unlock();
}
