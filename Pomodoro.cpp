/*
 * Pomodoro.cpp
 *
 *  Created on: 13/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#include <Pomodoro.h>
#include <QString>
#include <QDir>
#include <QFile>

//-----------------------------------------------------------------
Pomodoro::Pomodoro()
: m_pomodoroTime{25 * 60 * 1000}
, m_shortBreakTime{5 * 60 * 1000}
, m_longBreakTime{15 * 60 * 1000}
, m_numBeforeBreak{4}
, m_numPomodoros{0}
, m_numShortBreaks{0}
, m_numLongBreaks{0}
, m_progress{0}
, m_status{Status::Stopped}
, m_continuous{false}
, m_sessionPomodoros{12}
, m_useSounds{true}
, m_elapsedMSeconds{0}
{
	m_timer.setSingleShot(true);
	m_progressTimer.setSingleShot(false);
	connect(&m_progressTimer, SIGNAL(timeout()), this, SLOT(updateProgress()), Qt::QueuedConnection);

	QDir temporalPath = QDir::tempPath();

	// NOTE: Load sound files. QSound can´t play a file from the qt resource file
	// so we will dump them first to the temporal directory, then load the resources
	// and delete them.
	QString fileName = temporalPath.absolutePath() + QString("/tictac.wav");
	QFile::copy(":/DesktopCapture/sounds/tictac.wav", fileName);
	m_tictac = new QSound(fileName);
	QFile::remove(fileName);

	fileName = temporalPath.absolutePath() + QString("/crank.wav");
	QFile::copy(":/DesktopCapture/sounds/crank.wav", fileName);
	m_crank = new QSound(fileName);
	QFile::remove(fileName);

	fileName = temporalPath.absolutePath() + QString("/deskbell.wav");
	QFile::copy(":/DesktopCapture/sounds/deskbell.wav", fileName);
	m_ring = new QSound(fileName);
	QFile::remove(fileName);
}

//-----------------------------------------------------------------
Pomodoro::~Pomodoro()
{
	if (m_status != Status::Stopped)
		stop();

	// delete sounds
	delete m_tictac;
	delete m_crank;
	delete m_ring;
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
	m_startTime = QTime(0,0,0,0);
	m_startTime.start();
	m_progress = 0;
	m_elapsedMSeconds = 0;
	updateProgress();

	if (m_useSounds)
	{
		Sleeper::msleep(1300);
		m_crank->play();
		Sleeper::msleep(600);
		m_tictac->play();
	}
}

//-----------------------------------------------------------------
QTime Pomodoro::getPomodoroTime()
{
	QTime time = QTime(0, 0, 0, 0);
	time = time.addMSecs(m_pomodoroTime);
	return time;
}

//-----------------------------------------------------------------
QTime Pomodoro::getShortBreakTime()
{
	QTime time = QTime(0, 0, 0, 0);
	time = time.addMSecs(m_shortBreakTime);
	return time;
}

//-----------------------------------------------------------------
QTime Pomodoro::getLongBreakTime()
{
	QTime time = QTime(0, 0, 0, 0);
	time = time.addMSecs(m_longBreakTime);
	return time;
}

//-----------------------------------------------------------------
void Pomodoro::stopTimers()
{
	if (m_useSounds)
	{
		m_tictac->stop();
		m_ring->play();
	}

	m_timer.stop();
	m_progressTimer.stop();
	// disconnect respective signals in the caller.
}

//-----------------------------------------------------------------
void Pomodoro::startPomodoro()
{
	m_timer.setInterval(m_pomodoroTime);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(endPomodoro()), Qt::QueuedConnection);
	m_progressTimer.setInterval(m_pomodoroTime / 8);
	m_status = Status::Pomodoro;
	emit beginPomodoro();
	startTimers();
}

//-----------------------------------------------------------------
void Pomodoro::startShortBreak()
{
	m_timer.setInterval(m_shortBreakTime);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(endShortBreak()), Qt::QueuedConnection);
	m_progressTimer.setInterval(m_shortBreakTime / 8);
	m_status = Status::ShortBreak;
	emit beginShortBreak();
	startTimers();
}

//-----------------------------------------------------------------
void Pomodoro::startLongBreak()
{
	m_timer.setInterval(m_longBreakTime);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(endLongBreak()), Qt::QueuedConnection);
	m_progressTimer.setInterval(m_longBreakTime / 8);
	m_status = Status::LongBreak;
	emit beginLongBreak();
	startTimers();
}

//-----------------------------------------------------------------
void Pomodoro::start()
{
	Q_ASSERT(m_status == Status::Stopped);

	if (m_numPomodoros == 0 || ((m_numPomodoros == m_numShortBreaks) && (m_numPomodoros % m_numBeforeBreak != 0)))
		startPomodoro();
	else
	{
		if (m_numPomodoros % m_numBeforeBreak != 0)
			startShortBreak();
		else
			startLongBreak();
	}
}

//-----------------------------------------------------------------
void Pomodoro::pause(bool value)
{
	static Status oldStatus;

	if (Status::Stopped == m_status)
		return;

	if (Status::Paused != m_status)
	{
		// pause timers
		m_elapsedMSeconds += m_startTime.elapsed();
		oldStatus = m_status;
		m_status = Status::Paused;
		m_timer.stop();
		m_progressTimer.stop();
	}
	else
		if (Status::Paused == m_status)
		{
			// resume
			QTime time = QTime(0,0,0,0);
			time = time.addMSecs(m_elapsedMSeconds);
			unsigned long mSeconds, progressInterval, progressMSeconds;
			if (Status::Pomodoro == oldStatus)
			{
				mSeconds = time.msecsTo(getPomodoroTime());
				progressInterval = m_pomodoroTime / 8;
				progressMSeconds = m_pomodoroTime - m_elapsedMSeconds;
			}
			else
				if(Status::ShortBreak == oldStatus)
				{
					mSeconds = time.msecsTo(getShortBreakTime());
					progressInterval = m_shortBreakTime / 8;
					progressMSeconds = m_shortBreakTime - m_elapsedMSeconds;
				}
				else
					if(Status::LongBreak == oldStatus)
					{
						mSeconds = time.msecsTo(getLongBreakTime());
						progressInterval = m_longBreakTime / 8;
						progressMSeconds = m_longBreakTime - m_elapsedMSeconds;
					}

			m_status = oldStatus;

			m_startTime = QTime(0,0,0,0);
			m_startTime.start();
			m_timer.setInterval(mSeconds);
			m_timer.start();

			progressMSeconds = progressMSeconds % progressInterval;
			m_progressTimer.setInterval(progressMSeconds);
			m_progressTimer.start();
		}
}

//-----------------------------------------------------------------
unsigned int Pomodoro::elapsed()
{
	if (Status::Paused == m_status)
		return m_elapsedMSeconds;

	return m_elapsedMSeconds + m_startTime.elapsed();
}

//-----------------------------------------------------------------
void Pomodoro::stop()
{
	stopTimers();
	m_timer.disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endPomodoro()));
	m_timer.disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endLongBreak()));
	m_timer.disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endShortBreak()));
	m_numLongBreaks = 0;
	m_numShortBreaks = 0;
	m_numPomodoros = 0;
	m_progress = 0;
	m_status = Status::Stopped;
}

//-----------------------------------------------------------------
void Pomodoro::invalidate()
{
	if (Status::Stopped == m_status)
		return;

	stopTimers();
	m_timer.disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endPomodoro()));
	m_timer.disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endLongBreak()));
	m_timer.disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endShortBreak()));
	m_progress = 0;
	m_status = Status::Stopped;
	start();
}

//-----------------------------------------------------------------
void Pomodoro::setPomodoroTime(QTime duration)
{
	QTime zeroTime;
	m_pomodoroTime = zeroTime.msecsTo(duration);
}

//-----------------------------------------------------------------
void Pomodoro::setShortBreakTime(QTime duration)
{
	QTime zeroTime;
	m_shortBreakTime = zeroTime.msecsTo(duration);
}

//-----------------------------------------------------------------
void Pomodoro::setLongBreakTime(QTime duration)
{
	QTime zeroTime;
	m_longBreakTime = zeroTime.msecsTo(duration);
}

//-----------------------------------------------------------------
void Pomodoro::setTask(QString taskTitle)
{
	m_title = taskTitle;
}

//-----------------------------------------------------------------
void Pomodoro::updateProgress()
{
	unsigned long interval;
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
	++m_numPomodoros;

	if (m_numPomodoros == m_sessionPomodoros)
	{
		invalidate();
		emit sessionEnded();
		return;
	}

	stopTimers();
	disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endPomodoro()));

	emit pomodoroEnded();
	startShortBreak();
}

//-----------------------------------------------------------------
void Pomodoro::endShortBreak()
{
	++m_numShortBreaks;

	stopTimers();
	disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endShortBreak()));

	emit shortBreakEnded();

	if ((m_numPomodoros % m_numBeforeBreak) == 0)
		startLongBreak();
	else
		startPomodoro();
}

//-----------------------------------------------------------------
void Pomodoro::endLongBreak()
{
	++m_numLongBreaks;

	stopTimers();
	disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endLongBreak()));

	emit longBreakEnded();
	startPomodoro();
}

//-----------------------------------------------------------------
void Pomodoro::setContinuousTicTac(bool value)
{
	if (m_status == Status::Stopped)
	{
		m_continuous = value;
		m_tictac->setLoops(((value == true) ? -1 : 1));
	}
}

//-----------------------------------------------------------------
QIcon Pomodoro::icon()
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
QTime Pomodoro::completedSessionTime()
{
	QTime elapsedTime;
	unsigned int seconds = m_numPomodoros * m_pomodoroTime / 1000 ;
	seconds += m_numShortBreaks * m_shortBreakTime / 1000;
	seconds += m_numLongBreaks * m_longBreakTime / 1000;

	elapsedTime.addSecs(seconds);

	return elapsedTime;
}

//-----------------------------------------------------------------
QTime Pomodoro::elapsedTime()
{
	QTime returnTime = QTime(0,0,0,0);
	if (Status::Paused == m_status)
		returnTime = returnTime.addMSecs(m_elapsedMSeconds);
	else
		returnTime = returnTime.addMSecs(m_elapsedMSeconds + m_startTime.elapsed());

	return returnTime;
}
