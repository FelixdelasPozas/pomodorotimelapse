/*
 * PomodoroStatistics.cpp
 *
 *  Created on: 23/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#include <PomodoroStatistics.h>
#include <QKeyEvent>
#include <QObject>
#include <QEvent>
#include <QLineEdit>
#include <QInputDialog>
#include <QDebug>

//-----------------------------------------------------------------
PomodoroStatistics::PomodoroStatistics(Pomodoro* pomodoro, bool paused, QWidget* parent)
: QDialog(parent)
, m_pomodoro{pomodoro}
, m_paused{false}
, m_result{Result::None}
{
	setupUi(this);
	setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint| Qt::WindowSystemMenuHint);
	setWindowTitle("Pomodoro Statistics");
	setWindowIcon(QIcon(":/DesktopCapture/2.ico"));

	m_status->setTextFormat(Qt::RichText);
	updateGUI(m_pomodoro->status());

	connect(m_invalidate, SIGNAL(pressed()), this, SLOT(invalidate()));
	connect(m_stop, SIGNAL(pressed()), this, SLOT(stop()));
	connect(m_continue, SIGNAL(pressed()), this, SLOT(resume()));
	connect(m_pause, SIGNAL(pressed()), this, SLOT(pause()));

	if(m_pomodoro->status() != Pomodoro::Status::Stopped)
	{
		m_timer.setInterval(1000);
		m_timer.setSingleShot(false);
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateElapsedTime()), Qt::DirectConnection);
		connect(m_taskButton, SIGNAL(pressed()), this, SLOT(updateTask()), Qt::QueuedConnection);
		m_timer.start();
	}

	if(paused)
		pause();
}

//-----------------------------------------------------------------
PomodoroStatistics::~PomodoroStatistics()
{
	disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(updateElapsedTime()));
	m_timer.stop();
}

//-----------------------------------------------------------------
void PomodoroStatistics::invalidate()
{
	m_pomodoro->invalidateCurrent();
}

//-----------------------------------------------------------------
void PomodoroStatistics::stop()
{
	m_pomodoro->stop();
	m_result = Result::Stop;
	accept();
}

//-----------------------------------------------------------------
void PomodoroStatistics::resume()
{
	m_result = Result::Continue;
	accept();
}

//-----------------------------------------------------------------
void PomodoroStatistics::pause()
{
	if (!m_paused)
		m_pause->setText("Resume");
	else
		m_pause->setText("Pause");

	m_invalidate->setEnabled(m_paused);
	m_continue->setEnabled(m_paused);
	m_stop->setEnabled(m_paused);

	m_paused = !m_paused;
	m_pomodoro->pause();

	updateGUI(m_pomodoro->status());
}

//-----------------------------------------------------------------
void PomodoroStatistics::updateElapsedTime()
{
	if (m_pomodoro->status() == Pomodoro::Status::Stopped)
		m_elapsedTime->setText("--:--:--");
	else
	{
		QTime elapsedTime = m_pomodoro->elapsedTime();
		m_elapsedTime->setText(elapsedTime.toString(Qt::TextDate));
	}
	m_elapsedTime->repaint();
}

//-----------------------------------------------------------------
void PomodoroStatistics::updateGUI(Pomodoro::Status status)
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

	QTime elapsedTime = m_pomodoro->elapsedTime();
	m_elapsedTime->setText(elapsedTime.toString(Qt::TextDate));

	if (status == Pomodoro::Status::LongBreak)
		m_status->setText("<span style=\" color:#00ff00;\">In a long break</span>");
	else
		if (status == Pomodoro::Status::ShortBreak)
			m_status->setText("<span style=\" color:#0000ff;\">In a short break</span>");
		else
			if (status == Pomodoro::Status::Pomodoro)
				m_status->setText("<span style=\" color:#ff0000;\">In a pomodoro</span>");
			else
				if (status == Pomodoro::Status::Paused)
					m_status->setText("<span style=\" color:#000000;\">Paused</span>");
				else
					if (status == Pomodoro::Status::Stopped)
					{
						m_status->setText("<span style=\" color:#000000;\">Session finished</span>");
						m_elapsedTime->setText("--:--:--");
						m_invalidate->setEnabled(false);
						m_pause->setEnabled(false);
					}

	m_completedTime->setText(totalTime.toString(Qt::TextDate));

	m_taskName->setText(m_pomodoro->getTask());
	repaint();
}

//-----------------------------------------------------------------
void PomodoroStatistics::updateTask()
{
	 bool ok;
	 QString text = QInputDialog::getText(this,
			                                  tr("Enter task name"),
			                                  tr("Task name:"),
			                                  QLineEdit::Normal,
			                                  m_taskName->text(), &ok);
	 if (ok && !text.isEmpty())
	 {
		 m_taskName->setText(text);
		 m_pomodoro->setTask(m_taskName->text());
	 }
	 m_taskButton->setDown(false);
}
