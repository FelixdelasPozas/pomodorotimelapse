/*
 * PomodoroStatistics.h
 *
 *  Created on: 23/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef POMODORO_STATISTICS_H_
#define POMODORO_STATISTICS_H_

// Qt
#include <QDialog>
#include <QTimeLine>
#include <QTimer>

// Application
#include "Pomodoro.h"
#include "ui_PomodoroStatistics.h"

class PomodoroStatistics
: public QDialog
, public Ui_PomodoroStatistics
{
	Q_OBJECT
	public:
		explicit PomodoroStatistics(Pomodoro *pomodoro, QWidget *parent = nullptr);
		virtual ~PomodoroStatistics();

		void updateGUI();
	signals:
		void invalidated();
		void stopped();
		void paused();
		void resumed();

	public slots:
		void invalidate();
		void stop();
		void continuePomodoro();
		void pause();
		void updateElapsedTime();

	private:
		Pomodoro *m_pomodoro;
		QTimeLine m_timeLine;
		QTimer    m_timer;
		bool      m_paused;
};

#endif // POMODORO_STATISTICS_H_
