/*
 * Pomodoro.h
 *
 *  Created on: 13/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef POMODORO_H_
#define POMODORO_H_

#include <QTimer>
#include <QObject>
#include <QString>

class Pomodoro
: public QObject
{
	Q_OBJECT
	public:
		Pomodoro();
		virtual ~Pomodoro();

		void start();
		void pause();
		void stop();
		void invalidate();

		void setPomodoroTime(unsigned long seconds);
		void setShortBreakTime(unsigned long seconds);
		void setLongBreakTime(unsigned long seconds);
		void setTaskTitle(QString);

	signals:
		void beginPomodoro();
		void endPomodoro();
		void beginShortBreak();
		void endShortBreak();
		void beginLongBreak();
		void endLongBreak();

	private:
		unsigned int m_pomodoroTime;   // Duration of a pomodoro.
		unsigned int m_shortBreakTime; // Duration of a short break.
		unsigned int m_longBreakTime;  // Duration of a long break.
		unsigned int m_numBeforeBreak; // Number of pomodoros before a long break.
		unsigned int m_numPomodoros;   // Number of pomodoros completed in the session session.
		unsigned int m_numShortBreaks; // Number of short breaks in the session.
		unsigned int m_numLongBreaks;  // Number of long breaks in the session.
		QString      m_title;          // Title of the task.
		QTimer       m_timer;          // Timer.
};

#endif /* POMODORO_H_ */
