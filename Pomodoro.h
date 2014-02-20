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
		void progress(int);

	public slots:
	  void stopPomodoro();
	  void stopShortBreak();
	  void stopLongBreak();
	  void updateProgress();

	private:
		unsigned long long m_pomodoroTime;   // Duration of a pomodoro.
		unsigned long long m_shortBreakTime; // Duration of a short break.
		unsigned long long m_longBreakTime;  // Duration of a long break.
		unsigned long long m_numBeforeBreak; // Number of pomodoros before a long break.
		unsigned long long m_numPomodoros;   // Number of pomodoros completed in the session session.
		unsigned long long m_numShortBreaks; // Number of short breaks in the session.
		unsigned int       m_numLongBreaks;  // Number of long breaks in the session.
		QString            m_title;          // Title of the task.
		QTimer             m_timer;          // Timer.
		QTimer             m_progressTimer;  // Timer for progress.
		unsigned int       m_progress;       // Progress counter.
		unsigned long long m_remainingTime;  // Remaining time.
};

#endif /* POMODORO_H_ */
