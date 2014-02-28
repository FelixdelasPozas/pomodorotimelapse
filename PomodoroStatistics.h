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

class QObject;
class QEvent;

class PomodoroStatistics
: public QDialog
, public Ui_PomodoroStatistics
{
	Q_OBJECT
	public:
		explicit PomodoroStatistics(Pomodoro *pomodoro, QWidget *parent = nullptr);
		virtual ~PomodoroStatistics();

		void updateGUI(Pomodoro::Status status);

		enum class Result : std::int8_t { None = 1, Stop = 2, Continue = 3};
		Result getResult()
		{ return m_result; }

	public slots:
		void invalidate();
		void stop();
		void resume();
		void pause();
		void updateElapsedTime();
		void updateTask();

	private:
		Pomodoro *m_pomodoro;
		QTimeLine m_timeLine;
		QTimer    m_timer;
		bool      m_paused;
		Result    m_result;
};

#endif // POMODORO_STATISTICS_H_
