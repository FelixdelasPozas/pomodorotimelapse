/*
 * Pomodoro.cpp
 *
 *  Created on: 13/02/2014
 *      Author: Felix de las Pozas Alvarez
 */
#include <Pomodoro.h>
#include <QDebug>
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
{
	m_timer.setSingleShot(true);
	m_progressTimer.setSingleShot(false);
	connect(&m_progressTimer, SIGNAL(timeout()), this, SLOT(updateProgress()), Qt::QueuedConnection);
}

//-----------------------------------------------------------------
Pomodoro::~Pomodoro()
{
	stop();
}

//-----------------------------------------------------------------
void Pomodoro::start()
{
	if (m_numPomodoros % m_numBeforeBreak != 0)
	{
		m_timer.setInterval(m_pomodoroTime);
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(endPomodoro()), Qt::QueuedConnection);
		m_progressTimer.setInterval(m_pomodoroTime / 8);
		emit beginPomodoro();
	}
	else
	{
		m_timer.setInterval(m_shortBreakTime / 8);
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(endShortBreak()), Qt::QueuedConnection);
		m_progressTimer.setInterval(m_shortBreakTime / 8);
		emit beginShortBreak();
	}

	m_timer.start();
	m_progressTimer.start();
	m_progress = 0;
	emit progress(0);
}

//-----------------------------------------------------------------
void Pomodoro::stop()
{
	m_timer.stop();
	this->m_numLongBreaks = 0;
	this->m_numShortBreaks = 0;
	this->m_numPomodoros = 0;
	this->m_progress = 0;
}

//-----------------------------------------------------------------
void Pomodoro::invalidate()
{
	m_timer.stop();
	m_progress = 0;
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
	++m_progress;
	if (m_progress == 8)
	{
		m_progress = 0;
	}
	else
	{
		emit progress(m_progress);
		qDebug() << "progress" << m_progress;
	}


}

//-----------------------------------------------------------------
void Pomodoro::endPomodoro()
{
	qDebug() << "end pomodoro, num" << m_numPomodoros;
	m_timer.stop();
	m_progressTimer.stop();
	++m_numPomodoros;

	emit pomodoroEnded();

	disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endPomodoro()));

	m_timer.setInterval(m_shortBreakTime);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(endShortBreak()), Qt::QueuedConnection);
	m_progressTimer.setInterval(m_shortBreakTime / 8);

	m_timer.start();
	m_progressTimer.start();
	emit progress(0);
}

//-----------------------------------------------------------------
void Pomodoro::endShortBreak()
{
	qDebug() << "end short break, num" << this->m_numShortBreaks;
	m_timer.stop();
	m_progressTimer.stop();
	++m_numShortBreaks;

	emit shortBreakEnded();

	disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endShortBreak()));

	if ((m_numPomodoros % m_numBeforeBreak) == 0)
	{
		m_timer.setInterval(m_longBreakTime);
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(endLongBreak()), Qt::QueuedConnection);
		m_progressTimer.setInterval(m_longBreakTime / 8);
	}
	else
	{
		m_timer.setInterval(m_pomodoroTime / 8);
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(endPomodoro()), Qt::QueuedConnection);
		m_progressTimer.setInterval(m_pomodoroTime / 8);
	}

	m_timer.start();
	m_progressTimer.start();
	emit progress(0);
}

//-----------------------------------------------------------------
void Pomodoro::endLongBreak()
{
	qDebug() << "end long break, num" << this->m_numLongBreaks;
	m_timer.stop();
	m_progressTimer.stop();
	++this->m_numLongBreaks;

	emit longBreakEnded();

	disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(endLongBreak()));

	m_timer.setInterval(m_pomodoroTime / 8);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(endPomodoro()), Qt::QueuedConnection);
	m_progressTimer.setInterval(m_pomodoroTime / 8);

	m_timer.start();
	m_progressTimer.start();
	emit progress(0);
}
