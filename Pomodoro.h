/*
 * Pomodoro.h
 *
 *  Created on: 13/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef POMODORO_H_
#define POMODORO_H_

// Qt
#include <QTimer>
#include <QTime>
#include <QStringList>
#include <QObject>
#include <QString>
#include <QIcon>
#include <QSound>
#include <QThread>

// C++11
#include <cstdint>

class Sleeper
: public QThread
{
	public:
    static void usleep(unsigned long usecs)
    { QThread::usleep(usecs); }
    static void msleep(unsigned long msecs)
    { QThread::msleep(msecs); }
    static void sleep(unsigned long secs)
    { QThread::sleep(secs); }
};

class Pomodoro
: public QObject
{
	Q_OBJECT
	public:
		Pomodoro();
		virtual ~Pomodoro();

		void start();
		void pause(bool value);
		void stop();
		void invalidate();
		unsigned int elapsed();

		void startPomodoro();
		void startShortBreak();
		void startLongBreak();

		void setPomodoroTime(QTime time);
		void setShortBreakTime(QTime time);
		void setLongBreakTime(QTime time);
		void setTask(QString title);
		void setContinuousTicTac(bool value);
		void setSessionPodomodos(unsigned int value);
		void setUseSounds(bool value);
		QIcon icon();

		QTime getPomodoroTime();
		QTime getShortBreakTime();
		QTime getLongBreakTime();

		unsigned int completedPomodoros()
		{ return m_numPomodoros; };
		unsigned int completedShortBreaks()
		{ return m_numShortBreaks; };
		unsigned int completedLongBreaks()
		{	return m_numLongBreaks;	}
		QTime completedSessionTime();
		QTime elapsedTime();

	  enum class Status : std::int8_t { Pomodoro = 1, ShortBreak = 2, LongBreak = 3, Stopped = 4, Paused = 5 };
	  Status status()
	  { return m_status; };

	signals:
		void beginPomodoro();
		void pomodoroEnded();
		void beginShortBreak();
		void shortBreakEnded();
		void beginLongBreak();
		void longBreakEnded();
		void progress(unsigned int);
		void sessionEnded();

	public slots:
	  void updateProgress();
	  void endPomodoro();
	  void endShortBreak();
	  void endLongBreak();
	  void restoreOldProgressInterval();

	private:
	  void startTimers();
	  void stopTimers();

		unsigned long m_pomodoroTime;     // Duration of a pomodoro.
		unsigned long m_shortBreakTime;   // Duration of a short break.
		unsigned long m_longBreakTime;    // Duration of a long break.
		unsigned long m_numBeforeBreak;   // Number of pomodoros before a long break.
		unsigned long m_numPomodoros;     // Number of pomodoros completed in the session session.
		unsigned long m_numShortBreaks;   // Number of short breaks in the session.
		unsigned int  m_numLongBreaks;    // Number of long breaks in the session.
		QString       m_title;            // Title of the task.
		QTimer        m_timer;            // Timer.
		QTimer        m_progressTimer;    // Timer used for progress notifications in 1/8 of the time.
		unsigned int  m_progress;         // Progress counter.
		QStringList   m_completedTasks;   // List of tasks completed.
		QIcon         m_icon;             // Icon of actual situation.
		Status        m_status;           // Actual status of the pomodoro.
		bool          m_continuous;       // Continuous tic-tac sound.
		unsigned int  m_sessionPomodoros; // Number of pomodoros in a session.
		bool          m_useSounds;        // Use sounds.
		QTime         m_startTime;        // Start time of the last interval, used for pausing.
		unsigned int  m_elapsedMSeconds;  // Elapsed seconds since the last time m_timer started.

		QSound       *m_crank;
		QSound       *m_tictac;
		QSound       *m_ring;
};


#endif /* POMODORO_H_ */
