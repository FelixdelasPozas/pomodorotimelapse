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
{
}

//-----------------------------------------------------------------
Pomodoro::~Pomodoro()
{
}

//-----------------------------------------------------------------
void Pomodoro::start()
{
}

//-----------------------------------------------------------------
void Pomodoro::pause()
{
}

//-----------------------------------------------------------------
void Pomodoro::stop()
{
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
