/*
    File: Pomodoro.h
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

#ifndef POMODORO_H_
#define POMODORO_H_

// Qt
#include <QTimer>
#include <QTime>
#include <QStringList>
#include <QObject>
#include <QString>
#include <QIcon>
#include <QThread>
#include <QMap>

class QSound;
class QTemporaryFile;
class QTimerEvent;

// C++11
#include <cstdint>

/** \class Pomodoro
 *  \brief Implements the pomodoro technique using Qt timers
 *
 */
class Pomodoro
: public QObject
{
	Q_OBJECT
	public:
    /** \brief Pomodoro class constructor.
     *
     */
		Pomodoro();

    /** \brief Pomodoro class virtual destructor.
     *
     */
		virtual ~Pomodoro();

    /** \brief Clears the internal values and starts the session.
     *
     */
		void start();

    /** \brief Pauses the current unit.
     *
     */
		void pause();

    /** \brief Stops the session.
     *
     */
		void stop();

    /** \brief Restarts the current unit.
     *
     */
		void invalidateCurrent();

    /** \brief Returns the elapsed time of the unit in milliseconds.
     *
     */
		unsigned int elapsed() const;

    /** \brief Sets the duration of the pomodoro unit.
     * \param[in] time QTime of the duration.
     *
     */
		void setPomodoroDuration(QTime time);

    /** \brief Sets the duration of the short break unit.
     * \param[in] time QTime of the duration.
     *
     */
		void setShortBreakDuration(QTime time);

    /** \brief Sets the duration of the long break unit.
     * \param[in] time QTime of the duration.
     *
     */
		void setLongBreakDuration(QTime time);

    /** \brief Sets the name of the task for the current unit.
     * \param[in] title task description.
     *
     */
		void setTask(QString title);

    /** \brief Enables/disables the continuous tic-tac sound while the pomodoro is running.
     * \param[in] enabled boolean value
     *
     */
		void setContinuousTicTac(bool enabled);

    /** \brief Sets the number of pomodoros to complete per session.
     * \param[in] value
     *
     */
		void setSessionPodomodos(unsigned int value);

    /** \brief Sets the number of pomodoros to be completed before a long break.
     * \param[in] value
     *
     */
		void setPomodorosBeforeBreak(unsigned int value);

    /** \brief Enables/disables the sounds.
     * \param[in] enabled boolean value.
     *
     */
		void setUseSounds(bool enabled);

    /** \brief Returns the icon for the current unit and elapsed time.
     *
     */
		QIcon icon() const;

    /** \brief Returns the duration of a pomodoro unit.
     *
     */
		QTime getPomodoroDuration() const;

		/** \brief Returns the duration of a short break unit.
     *
     */
		QTime getShortBreakDuration() const;

    /** \brief Returns the duration of a long break unit.
     *
     */
		QTime getLongBreakDuration() const;

    /** \brief Returns the number of pomodoros to be completed before a long break.
     *
     */
		unsigned long getPomodorosBeforeLongBreak() const
		{ return m_numBeforeBreak; }

    /** \brief Returns the number of pomodoros of a session.
     *
     */
		unsigned int getPomodorosInSession() const
		{ return m_sessionPomodoros; };

    /** \brief Returns the current task.
     *
     */
		QString getTask() const
		{ return m_task; };

    /** \brief Returns the completed pomodoros and their tasks.
     *
     */
		QMap<int, QString> getCompletedTasks() const
		{ return m_completedTasks; };

    /** \brief Returns the number of completed pomodoros in the session.
     *
     */
		unsigned int completedPomodoros() const
		{ return m_numPomodoros; };

    /** \brief Returns the number of completed short breaks in the session.
     *
     */
		unsigned int completedShortBreaks() const
		{ return m_numShortBreaks; };

    /** \brief Returns the number of completed long breaks in the session.
     *
     */
		unsigned int completedLongBreaks() const
		{	return m_numLongBreaks;	}

    /** \brief Returns the total time of completed units in the session.
     *
     */
		QTime completedSessionTime() const;

		/** \brief Returns the total time of the session;
		 *
		 */
		QTime sessionTime() const;

    /** \brief Returns the time elapsed since the beginning of the current unit.
     *
     */
		QTime elapsedTime();

    /** \brief Resets the session.
     *
     */
		void clear();

    enum class Status: char { Pomodoro = 1, ShortBreak, LongBreak, Stopped, Paused };

    /** \brief Returns the current status of the session (Stopped/Paused/Current unit).
     *
     */
	  Status status()
	  { return m_status; };

    /** \brief Returns a description of the current status (Stopped/Paused/Current unit).
     *
     */
	  QString statusMessage();

	signals:
		void beginPomodoro();
		void pomodoroEnded();
		void beginShortBreak();
		void shortBreakEnded();
		void beginLongBreak();
		void longBreakEnded();
		void progress(unsigned int);
		void sessionEnded();

	protected:
		void timerEvent(QTimerEvent *e);

	private slots:
    /** \brief Updates the pomodoro icon for the completed unit intervals.
     *
     */
	  void updateProgress();

    /** \brief Ends the pomodoro and starts a short break or a long break depending in the number of pomodoros per long break.
     *
     */
	  void endPomodoro();

    /** \brief Ends a short break and starts a pomodoro.
     *
     */
	  void endShortBreak();

    /** \brief Ends a long break and starts a pomodoro.
     *
     */
	  void endLongBreak();

	private:
    enum class Sound { CRANK, TICTAC, RING, NONE };
    static const int LENGTH_CRANK  = 530;
    static const int LENGTH_TICTAC = 450;
    static const int LENGTH_RING   = 1300;

    /** \brief Queues the sound to play.
     * \param[in] sound.
     *
     */
    void queueSound(Sound sound);

    /** \brief Plays a queued sound.
     *
     */
    void playNextSound();

    /** \brief Starts a pomodoro.
     *
     */
    void startPomodoro();

    /** \brief Starts a short break.
     *
     */
    void startShortBreak();

    /** \brief Starts a long break.
     *
     */
    void startLongBreak();

    /** \brief Starts the timers for the current unit.
     *
     */
	  void startTimers();

	  /** \brief Stops the timers for the current unit.
     *
     */
	  void stopTimers();

    unsigned long m_pomodoroTime;        /** Duration of a pomodoro.                                        */
    unsigned long m_shortBreakTime;      /** Duration of a short break.                                     */
    unsigned long m_longBreakTime;       /** Duration of a long break.                                      */
    unsigned long m_numBeforeBreak;      /** Number of pomodoros before a long break.                       */
    unsigned long m_numPomodoros;        /** Number of pomodoros completed in the session session.          */
    unsigned long m_numShortBreaks;      /** Number of short breaks in the session.                         */
    unsigned int  m_numLongBreaks;       /** Number of long breaks in the session.                          */
    QString       m_task;                /** Title of the task.                                             */
    QTimer        m_timer;               /** Timer.                                                         */
    QTimer        m_progressTimer;       /** Timer used for progress notifications in 1/8 of the unit time. */
    unsigned int  m_progress;            /** Progress counter.                                              */
    QIcon         m_icon;                /** Icon of actual situation.                                      */
    Status        m_status;              /** Actual status of the pomodoro.                                 */
    bool          m_continuousTicTac;    /** Continuous tic-tac sound.                                      */
    unsigned int  m_sessionPomodoros;    /** Number of pomodoros in a session.                              */
    bool          m_useSounds;           /** Use sounds.                                                    */
    QTime         m_startTime;           /** Start time of the last interval, used for pausing.             */
    unsigned int  m_elapsedMSeconds;     /** Elapsed seconds since the last time m_timer started.           */
    QMap<int, QString> m_completedTasks; /** Task names of completed pomodoros.                             */

    QSound *m_crank;  /** Crank sound      */
    QSound *m_tictac; /** Tic-tac sound    */
    QSound *m_ring;   /** Alarm ring sound */

    QTemporaryFile *m_tictac_file; /** Crank temporary file      */
    QTemporaryFile *m_crank_file;  /** Tic-tac temporary file    */
    QTemporaryFile *m_ring_file;   /** Alarm ring temporary file */

    QList<Sound> m_playList;
};


#endif /* POMODORO_H_ */
