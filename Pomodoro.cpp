/*
    File: Pomodoro.cpp
    Created on: 13/02/2014
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
#include <Pomodoro.h>

// Qt
#include <QString>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>
#include <QTimerEvent>
#include <QWidget>

//-----------------------------------------------------------------
Pomodoro::Pomodoro()
: QObject{}
, m_pomodoroTime    {25 * 60 * 1000}
, m_shortBreakTime  {5 * 60 * 1000}
, m_longBreakTime   {15 * 60 * 1000}
, m_numBeforeBreak  {4}
, m_numPomodoros    {0}
, m_numShortBreaks  {0}
, m_numLongBreaks   {0}
, m_task            {"Undefined task"}
, m_progress        {0}
, m_status          {Status::Stopped}
, m_continuousTicTac{false}
, m_sessionPomodoros{12}
, m_useSounds       {true}
, m_elapsedMSeconds {0}
{
	m_timer.setSingleShot(true);
	m_progressTimer.setSingleShot(false);

	connect(&m_progressTimer, SIGNAL(timeout()),
	        this,             SLOT(updateProgress()), Qt::QueuedConnection);

  m_tictac.setSource(QUrl::fromLocalFile(":/DesktopCapture/sounds/tictac.wav"));
  m_crank.setSource(QUrl::fromLocalFile(":/DesktopCapture/sounds/crank.wav"));
  m_ring.setSource(QUrl::fromLocalFile(":/DesktopCapture/sounds/deskbell.wav"));
}

//-----------------------------------------------------------------
Pomodoro::~Pomodoro()
{
	if (m_status != Status::Stopped)
		stop();
}

//-----------------------------------------------------------------
void Pomodoro::setUseSounds(bool value)
{
	if (m_status == Status::Stopped)
		m_useSounds = value;
}

//-----------------------------------------------------------------
void Pomodoro::startTimers()
{
	m_timer.start();
	m_progressTimer.start();
	m_startTime = QDateTime::currentDateTime();
	m_progress = 0;
	m_elapsedMSeconds = 0;
	updateProgress();

	if (m_useSounds)
	{
	  queueSound(Sound::CRANK);
	  queueSound(Sound::TICTAC);
	}
}

//-----------------------------------------------------------------
QTime Pomodoro::getPomodoroDuration() const
{
	QTime time{0,0,0,0};
	time = time.addMSecs(m_pomodoroTime);
	return time;
}

//-----------------------------------------------------------------
QTime Pomodoro::getShortBreakDuration() const
{
  QTime time{0,0,0,0};
	time = time.addMSecs(m_shortBreakTime);
	return time;
}

//-----------------------------------------------------------------
QTime Pomodoro::getLongBreakDuration() const
{
  QTime time{0,0,0,0};
	time = time.addMSecs(m_longBreakTime);
	return time;
}

//-----------------------------------------------------------------
void Pomodoro::stopTimers()
{
	// disconnect respective signals in the caller of this one.
	m_timer.stop();
	m_progressTimer.stop();

	bool noSounds = (m_status == Status::Stopped || m_status == Status::Paused);
	if (m_useSounds && !noSounds)
	{
		if (m_continuousTicTac)
		  queueSound(Sound::NONE);

		queueSound(Sound::RING);
	}
}

//-----------------------------------------------------------------
void Pomodoro::startPomodoro()
{
	m_timer.setInterval(m_pomodoroTime);

	connect(&m_timer, SIGNAL(timeout()),
	        this,     SLOT(endPomodoro()), Qt::QueuedConnection);

	m_progressTimer.setInterval(m_pomodoroTime / 8);
	m_status = Status::Pomodoro;

	emit beginPomodoro();

	startTimers();
}

//-----------------------------------------------------------------
void Pomodoro::startShortBreak()
{
	m_timer.setInterval(m_shortBreakTime);

	connect(&m_timer, SIGNAL(timeout()),
	        this,     SLOT(endShortBreak()), Qt::QueuedConnection);

	m_progressTimer.setInterval(m_shortBreakTime / 8);
	m_status = Status::ShortBreak;

	emit beginShortBreak();

	startTimers();
}

//-----------------------------------------------------------------
void Pomodoro::startLongBreak()
{
	m_timer.setInterval(m_longBreakTime);

	connect(&m_timer, SIGNAL(timeout()),
	        this,     SLOT(endLongBreak()), Qt::QueuedConnection);

	m_progressTimer.setInterval(m_longBreakTime / 8);
	m_status = Status::LongBreak;

	emit beginLongBreak();

	startTimers();
}

//-----------------------------------------------------------------
void Pomodoro::start()
{
	Q_ASSERT(m_status == Status::Stopped);

	m_numLongBreaks = 0;
	m_numShortBreaks = 0;
	m_numPomodoros = 0;
	m_progress = 0;
	m_completedTasks.clear();

	startPomodoro();
}

//-----------------------------------------------------------------
void Pomodoro::pause()
{
	if (Status::Stopped == m_status) return;

	static Status oldStatus;

	if (Status::Paused != m_status)
	{
		// pause timers
		m_elapsedMSeconds += m_startTime.msecsTo(QDateTime::currentDateTime());
		oldStatus = m_status;
		m_status = Status::Paused;
		m_timer.stop();
		m_progressTimer.stop();
		if (m_continuousTicTac)
		  queueSound(Sound::NONE);
	}
	else
	{
		// resume
	  QTime time{0,0,0,0};
		time = time.addMSecs(m_elapsedMSeconds);
		unsigned long mSeconds = 0;
		unsigned long progressInterval = 0;
		unsigned long progressMSeconds = 0;

		switch(oldStatus)
		{
		  case Status::Pomodoro:
	      mSeconds = time.msecsTo(getPomodoroDuration());
	      progressInterval = m_pomodoroTime / 8;
	      progressMSeconds = m_pomodoroTime - m_elapsedMSeconds;
		    break;
		  case Status::ShortBreak:
        mSeconds = time.msecsTo(getShortBreakDuration());
        progressInterval = m_shortBreakTime / 8;
        progressMSeconds = m_shortBreakTime - m_elapsedMSeconds;
		    break;
		  case Status::LongBreak:
        mSeconds = time.msecsTo(getLongBreakDuration());
        progressInterval = m_longBreakTime / 8;
        progressMSeconds = m_longBreakTime - m_elapsedMSeconds;
		    break;
		  default:
		    Q_ASSERT(false);
		}

		m_status = oldStatus;

		m_startTime = QDateTime::currentDateTime();
		m_timer.setInterval(mSeconds);
		m_timer.start();

		progressMSeconds = progressMSeconds % progressInterval;
		m_progressTimer.setInterval(progressMSeconds);
		m_progressTimer.start();

		if (m_continuousTicTac)
		  queueSound(Sound::TICTAC);
	}
}

//-----------------------------------------------------------------
unsigned int Pomodoro::elapsed() const
{
	if (Status::Paused == m_status)
		return m_elapsedMSeconds;

	return m_elapsedMSeconds + m_startTime.msecsTo(QDateTime::currentDateTime());
}

//-----------------------------------------------------------------
void Pomodoro::stop()
{
	if (Status::Stopped == m_status) return;

	stopTimers();

	switch(m_status)
	{
		case Status::Pomodoro:
			m_timer.disconnect(&m_timer, SIGNAL(timeout()),
			                   this,     SLOT(endPomodoro()));
			break;
		case Status::ShortBreak:
			m_timer.disconnect(&m_timer, SIGNAL(timeout()),
			                   this,     SLOT(endShortBreak()));
			break;
		case Status::LongBreak:
			m_timer.disconnect(&m_timer, SIGNAL(timeout()),
			                   this,     SLOT(endLongBreak()));
			break;
		case Status::Paused:
			pause();
			stop();
			break;
		default:
			Q_ASSERT(false);
			break;
	}

	m_status = Status::Stopped;
}

//-----------------------------------------------------------------
void Pomodoro::invalidateCurrent()
{
	if (Status::Stopped == m_status) return;

	if(Status::Paused != m_status)
	  stopTimers();

	switch(m_status)
	{
		case Status::Pomodoro:
			m_timer.disconnect(&m_timer, SIGNAL(timeout()),
			                   this,     SLOT(endPomodoro()));
			startPomodoro();
			break;
		case Status::ShortBreak:
			m_timer.disconnect(&m_timer, SIGNAL(timeout()),
			                   this,     SLOT(endShortBreak()));
			startShortBreak();
			break;
		case Status::LongBreak:
			m_timer.disconnect(&m_timer, SIGNAL(timeout()),
			                   this,     SLOT(endLongBreak()));
			startLongBreak();
			break;
		case Status::Paused:
			pause();
			invalidateCurrent();
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------
void Pomodoro::setPomodoroDuration(QTime duration)
{
	QTime zeroTime{0,0,0,0};
	m_pomodoroTime = zeroTime.msecsTo(duration);
}

//-----------------------------------------------------------------
void Pomodoro::setShortBreakDuration(QTime duration)
{
  QTime zeroTime{0,0,0,0};
	m_shortBreakTime = zeroTime.msecsTo(duration);
}

//-----------------------------------------------------------------
void Pomodoro::setLongBreakDuration(QTime duration)
{
  QTime zeroTime{0,0,0,0};
	m_longBreakTime = zeroTime.msecsTo(duration);
}

//-----------------------------------------------------------------
void Pomodoro::setTaskTitle(QString taskTitle)
{
	m_task = taskTitle;
}

//-----------------------------------------------------------------
void Pomodoro::updateProgress()
{
	unsigned long interval = 0;
	switch(m_status)
	{
		case Status::Pomodoro:
			m_icon = QIcon(QString(":/DesktopCapture/%1-red.ico").arg(m_progress));
			interval = m_pomodoroTime / 8;
			break;
		case Status::ShortBreak:
			m_icon = QIcon(QString(":/DesktopCapture/%1-blue.ico").arg(m_progress));
			interval = m_shortBreakTime / 8;
			break;
		case Status::LongBreak:
			m_icon = QIcon(QString(":/DesktopCapture/%1-green.ico").arg(m_progress));
			interval = m_longBreakTime / 8;
			break;
		default:
			Q_ASSERT(false);
			break;
	}

	if (m_progressTimer.interval() < static_cast<int>(interval))
	{
		m_progressTimer.stop();
		m_progressTimer.setInterval(interval);
		m_progressTimer.start();
	}

	emit progress(m_progress);

	++m_progress;
}

//-----------------------------------------------------------------
void Pomodoro::endPomodoro()
{
	m_completedTasks[m_numPomodoros] = m_task;

	++m_numPomodoros;

	if (m_numPomodoros == m_sessionPomodoros)
	{
		stop();
		m_icon = QIcon(QString(":/DesktopCapture/0.ico"));
		emit sessionEnded();
		return;
	}

	stopTimers();
	disconnect(&m_timer, SIGNAL(timeout()),
	           this,     SLOT(endPomodoro()));

	emit pomodoroEnded();

  if ((m_numPomodoros % m_numBeforeBreak) == 0)
    startLongBreak();
  else
    startShortBreak();
}

//-----------------------------------------------------------------
void Pomodoro::endShortBreak()
{
	++m_numShortBreaks;

	stopTimers();
	disconnect(&m_timer, SIGNAL(timeout()),
	           this,     SLOT(endShortBreak()));

	emit shortBreakEnded();
	startPomodoro();
}

//-----------------------------------------------------------------
void Pomodoro::endLongBreak()
{
	++m_numLongBreaks;

	stopTimers();
	disconnect(&m_timer, SIGNAL(timeout()),
	           this,     SLOT(endLongBreak()));

	emit longBreakEnded();
	startPomodoro();
}

//-----------------------------------------------------------------
void Pomodoro::setContinuousTicTac(bool value)
{
	if (m_status == Status::Stopped)
	{
		m_continuousTicTac = value;
		m_tictac.setLoopCount((value ? QSoundEffect::Infinite : 1));
	}
}

//-----------------------------------------------------------------
QIcon Pomodoro::icon() const
{
	return m_icon;
}

//-----------------------------------------------------------------
void Pomodoro::setSessionPodomodos(unsigned int value)
{
	if (m_status == Status::Stopped)
		m_sessionPomodoros = value;
}

//-----------------------------------------------------------------
void Pomodoro::setPomodorosBeforeBreak(unsigned int value)
{
	if (m_status == Status::Stopped)
		m_numBeforeBreak = value;
}

//-----------------------------------------------------------------
QTime Pomodoro::completedSessionTime() const
{
	QTime elapsedTime{0,0,0,0};
	unsigned long seconds = m_numPomodoros * m_pomodoroTime / 1000 ;
	seconds += m_numShortBreaks * m_shortBreakTime / 1000;
	seconds += m_numLongBreaks * m_longBreakTime / 1000;

	return elapsedTime.addSecs(seconds);
}

//-----------------------------------------------------------------
QTime Pomodoro::sessionTime() const
{
  QTime sessionTime{0,0,0,0};
  unsigned long seconds = m_sessionPomodoros * m_pomodoroTime / 1000;
  const auto numLongBreaks = (m_sessionPomodoros / m_numBeforeBreak) - ((m_sessionPomodoros % m_numBeforeBreak == 0) ? 1 : 0);
  seconds += numLongBreaks * m_longBreakTime / 1000;
  seconds += (m_sessionPomodoros - numLongBreaks - 1) * m_shortBreakTime / 1000;

  sessionTime = sessionTime.addSecs(seconds);

  return sessionTime;
}

//-----------------------------------------------------------------
QTime Pomodoro::elapsedTime() const
{
	QTime returnTime{0,0,0,0};
	if (Status::Paused == m_status)
		returnTime = returnTime.addMSecs(m_elapsedMSeconds);
	else
	{
	  if(Status::Stopped != m_status)
	    returnTime = returnTime.addMSecs(m_elapsedMSeconds + m_startTime.msecsTo(QDateTime::currentDateTime()));
	}

	return returnTime;
}

//-----------------------------------------------------------------
void Pomodoro::clear()
{
	m_numPomodoros = 0;
	m_numShortBreaks = 0;
	m_numLongBreaks = 0;
}

//-----------------------------------------------------------------
QString Pomodoro::statusMessage()
{
	QString returnVal;

	switch(m_status)
	{
		case Status::Stopped:
			returnVal = QString("Stopped.");
			break;
		case Status::Pomodoro:
			returnVal = QString("In a pomodoro.");
			break;
		case Status::ShortBreak:
			returnVal = QString("In a short break.");
			break;
		case Status::LongBreak:
			returnVal = QString("In a long break.");
			break;
		case Status::Paused:
			returnVal = QString("Paused.");
			break;
		default:
			Q_ASSERT(false);
			break;
	}

	return returnVal;
}

//-----------------------------------------------------------------
void Pomodoro::queueSound(Sound sound)
{
  m_playList << sound;

  if(m_playList.size() == 1)
    playNextSound();
}

//-----------------------------------------------------------------
void Pomodoro::playNextSound()
{
  switch(m_playList.first())
  {
    case Sound::CRANK:
      startTimer(LENGTH_CRANK);
      m_crank.play();
      break;
    case Sound::RING:
      startTimer(LENGTH_RING);
      m_ring.play();
      break;
    case Sound::TICTAC:
      startTimer(LENGTH_TICTAC);
      m_tictac.play();
      break;
    case Sound::NONE:
      if(m_tictac.loopCount() == QSoundEffect::Infinite)
      {
        startTimer(LENGTH_TICTAC);
        m_tictac.stop();
      }
      break;
    default:
      break;
  }
}

//-----------------------------------------------------------------
void Pomodoro::timerEvent(QTimerEvent *e)
{
  killTimer(e->timerId());
  m_playList.removeFirst();

  if(!m_playList.empty())
    playNextSound();
}
