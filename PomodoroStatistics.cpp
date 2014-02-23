/*
 * PomodoroStatistics.cpp
 *
 *  Created on: 23/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#include <PomodoroStatistics.h>
#include <QDebug>

//-----------------------------------------------------------------
PomodoroStatistics::PomodoroStatistics(Pomodoro* pomodoro, QWidget* parent)
: QDialog(parent)
, m_pomodoro{pomodoro}
, m_paused{false}
{
	setupUi(this);
	setWindowTitle("Pomodoro Statistics");
	setWindowIcon(QIcon(":/DesktopCapture/2.ico"));

	updateGUI();

	connect(m_invalidate, SIGNAL(pressed()), this, SLOT(invalidate()));
	connect(m_stop, SIGNAL(pressed()), this, SLOT(stop()));
	connect(m_continue, SIGNAL(pressed()), this, SLOT(continuePomodoro()));
	connect(m_pause, SIGNAL(pressed()), this, SLOT(pause()));

	m_timer.setInterval(1000);
	m_timer.setSingleShot(false);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateElapsedTime()), Qt::DirectConnection);
	m_timer.start();
}

//-----------------------------------------------------------------
PomodoroStatistics::~PomodoroStatistics()
{
	m_timer.stop();
}

//-----------------------------------------------------------------
void PomodoroStatistics::invalidate()
{
	emit invalidated();
}

//-----------------------------------------------------------------
void PomodoroStatistics::stop()
{
	emit stop();
	done(QDialog::Accepted);
}

//-----------------------------------------------------------------
void PomodoroStatistics::continuePomodoro()
{
	done(QDialog::Accepted);
}

//-----------------------------------------------------------------
void PomodoroStatistics::pause()
{
	if (!m_paused)
	{
		m_paused = true;
		emit paused();
		m_pause->setText("Resume");
		m_elapsedTime->setDisabled(false);
	}
	else
	{
		m_paused = false;
		emit resumed();
		m_pause->setText("Pause");
		m_elapsedTime->setDisabled(true);
	}

	updateGUI();
}

//-----------------------------------------------------------------
void PomodoroStatistics::updateElapsedTime()
{
	QTime elapsedTime = m_pomodoro->elapsedTime();
	m_elapsedTime->setText(elapsedTime.toString(Qt::TextDate));
	m_elapsedTime->repaint();
}

//-----------------------------------------------------------------
void PomodoroStatistics::updateGUI()
{
	unsigned long totalMs = 0;
	QTime pomodoroTime = QTime(0, 0, 0, 0);

	unsigned long seconds = pomodoroTime.secsTo(m_pomodoro->getPomodoroTime());
	seconds *= m_pomodoro->completedPomodoros();
	totalMs += seconds;
	pomodoroTime = pomodoroTime.addSecs(seconds);

	m_completedPomodoros->setText(QString("%1    (%2)").arg(m_pomodoro->completedPomodoros()).arg(pomodoroTime.toString(Qt::TextDate)));

	QTime shortBreakTime = QTime(0, 0, 0, 0);
	seconds = shortBreakTime.secsTo(m_pomodoro->getLongBreakTime());
	seconds *= m_pomodoro->completedShortBreaks();
	totalMs += seconds;
	shortBreakTime = shortBreakTime.addSecs(seconds);

	m_completedShortBreaks->setText(QString("%1    (%2)").arg(m_pomodoro->completedShortBreaks()).arg(shortBreakTime.toString(Qt::TextDate)));

	QTime longBreakTime = QTime(0, 0, 0, 0);
	seconds = longBreakTime.secsTo(m_pomodoro->getLongBreakTime());
	seconds *= m_pomodoro->completedLongBreaks();
	totalMs += seconds;
	longBreakTime = longBreakTime.addSecs(seconds);

	m_completedLongBreaks->setText(QString("%1    (%2)").arg(m_pomodoro->completedLongBreaks()).arg(longBreakTime.toString(Qt::TextDate)));

	QTime totalTime = QTime(0, 0, 0, 0);
	totalTime = totalTime.addSecs(totalMs);

	m_completedTime->setText(totalTime.toString(Qt::TextDate));

	QTime elapsedTime = m_pomodoro->elapsedTime();
	m_elapsedTime->setText(elapsedTime.toString(Qt::TextDate));

	m_paused = false;
	if (m_pomodoro->status() == Pomodoro::Status::LongBreak)
		m_status->setText("In a long break");
	else
		if (m_pomodoro->status() == Pomodoro::Status::ShortBreak)
			m_status->setText("In a short break");
		else
			if (m_pomodoro->status() == Pomodoro::Status::Pomodoro)
				m_status->setText("In a pomodoro");
			else
				if (m_pomodoro->status() == Pomodoro::Status::Paused)
				{
					m_status->setText("Paused");
					m_paused = true;
				}
				else
					if (m_pomodoro->status() == Pomodoro::Status::Stopped)
						m_status->setText("Stopped");

}
