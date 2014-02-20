/*
 * Pomodoro.cpp
 *
 *  Created on: 13/02/2014
 *      Author: Felix de las Pozas Alvarez
 */
#include <Pomodoro.h>

//-----------------------------------------------------------------
Pomodoro::Pomodoro()
: m_pomodoroTime{0}
, m_shortBreakTime{0}
, m_longBreakTime{0}
, m_numBeforeBreak{0}
, m_numPomodoros{0}
, m_numShortBreaks{0}
, m_numLongBreaks{0}
, m_progress{0}
, m_remainingTime{0}
{
}

//-----------------------------------------------------------------
Pomodoro::~Pomodoro()
{
	stop();
}

//-----------------------------------------------------------------
void Pomodoro::start()
{
	m_timer.setInterval(m_pomodoroTime);
	m_timer.setSingleShot(true);

	m_progressTimer.setInterval(m_pomodoroTime / 8);
	m_progressTimer.setSingleShot(false);

	m_progress = 0;
}

//-----------------------------------------------------------------
void Pomodoro::stop()
{
	m_timer.stop();
	m_progressTimer.stop();
}

//-----------------------------------------------------------------
void Pomodoro::invalidate()
{
}

//-----------------------------------------------------------------
void Pomodoro::setPomodoroTime(unsigned long seconds)
{
}

//-----------------------------------------------------------------
void Pomodoro::setShortBreakTime(unsigned long seconds)
{
}

//-----------------------------------------------------------------
void Pomodoro::setLongBreakTime(unsigned long seconds)
{
}

//-----------------------------------------------------------------
void Pomodoro::setTaskTitle(QString qString)
{
}

//-----------------------------------------------------------------
void Pomodoro::stopPomodoro()
{
}

//-----------------------------------------------------------------
void Pomodoro::stopShortBreak()
{
}

//-----------------------------------------------------------------
void Pomodoro::stopLongBreak()
{
}

//-----------------------------------------------------------------
void Pomodoro::updateProgress()
{
}
