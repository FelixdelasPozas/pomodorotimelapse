/*
 * Pomodoro.h
 *
 *  Created on: 13/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef POMODORO_H_
#define POMODORO_H_

#include <QTimer>
#include <QTime>
#include <QStringList>
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

		void setPomodoroTime(QTime time);
		void setShortBreakTime(QTime time);
		void setLongBreakTime(QTime time);
		void setTask(QString title);

	signals:
		void beginPomodoro();
		void pomodoroEnded();
		void beginShortBreak();
		void shortBreakEnded();
		void beginLongBreak();
		void longBreakEnded();
		void progress(unsigned int);

	public slots:
	  void updateProgress();
	  void endPomodoro();
	  void endShortBreak();
	  void endLongBreak();

	private:
		unsigned long m_pomodoroTime;   // Duration of a pomodoro.
		unsigned long m_shortBreakTime; // Duration of a short break.
		unsigned long m_longBreakTime;  // Duration of a long break.
		unsigned long m_numBeforeBreak; // Number of pomodoros before a long break.
		unsigned long m_numPomodoros;   // Number of pomodoros completed in the session session.
		unsigned long m_numShortBreaks; // Number of short breaks in the session.
		unsigned int  m_numLongBreaks;  // Number of long breaks in the session.
		QString       m_title;          // Title of the task.
		QTimer        m_timer;          // Timer.
		QTimer        m_progressTimer;  // Timer used for progress notifications in 1/8 of the time.
		unsigned int  m_progress;       // Progress counter.
		QStringList   m_completedTasks; // List of tasks completed.
};

unsigned long toNanoseconds(QTime time);

#endif /* POMODORO_H_ */
