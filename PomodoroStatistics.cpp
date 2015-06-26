/*
    File: PomodoroStatistics.cpp
    Created on: 23/02/2014
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

// Project
#include <PomodoroStatistics.h>

// Qt
#include <QKeyEvent>
#include <QObject>
#include <QEvent>
#include <QLineEdit>
#include <QInputDialog>
#include <QPainter>
#include <QFontMetrics>
#include <QDebug>

//-----------------------------------------------------------------
PomodoroStatistics::PomodoroStatistics(std::shared_ptr<Pomodoro> pomodoro, QWidget* parent)
: QDialog(parent)
, m_pomodoro{pomodoro}
, m_paused{pomodoro->status() == Pomodoro::Status::Paused}
, m_result{Result::None}
{
	setupUi(this);
	setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint| Qt::WindowSystemMenuHint);
	setWindowTitle("Pomodoro Statistics");
	setWindowIcon(QIcon(":/DesktopCapture/2.ico"));

	m_status->setTextFormat(Qt::RichText);

  connect(m_pomodoro.get(), SIGNAL(beginPomodoro()),
          this,             SLOT(updateGUI()));

  connect(m_pomodoro.get(), SIGNAL(beginShortBreak()),
          this,             SLOT(updateGUI()));

  connect(m_pomodoro.get(), SIGNAL(beginLongBreak()),
          this,             SLOT(updateGUI()));

  connect(m_pomodoro.get(), SIGNAL(sessionEnded()),
          this,             SLOT(updateGUI()));

	connect(m_invalidate, SIGNAL(pressed()),
	         this,         SLOT(invalidate()));

	connect(m_stop, SIGNAL(pressed()),
	         this,   SLOT(stop()));

	connect(m_continue, SIGNAL(pressed()),
	         this,       SLOT(resume()));

	connect(m_pause, SIGNAL(pressed()),
	         this,   SLOT(pause()));

	if(m_pomodoro->status() != Pomodoro::Status::Stopped)
	{		m_timer.setInterval(1000);
		m_timer.setSingleShot(false);

		connect(&m_timer, SIGNAL(timeout()),
		        this,     SLOT(updateElapsedTime()), Qt::DirectConnection);

		connect(m_taskButton, SIGNAL(pressed()),
		        this,         SLOT(updateTask()), Qt::QueuedConnection);

		m_timer.start();
	}

	updateGUI();
}

//-----------------------------------------------------------------
PomodoroStatistics::~PomodoroStatistics()
{
  disconnect(m_pomodoro.get(), SIGNAL(beginPomodoro()),
          this,                SLOT(updateGUI()));

  disconnect(m_pomodoro.get(), SIGNAL(beginShortBreak()),
          this,                SLOT(updateGUI()));

  disconnect(m_pomodoro.get(), SIGNAL(beginLongBreak()),
          this,                SLOT(updateGUI()));

  disconnect(m_pomodoro.get(), SIGNAL(sessionEnded()),
          this,                SLOT(updateGUI()));

	disconnect(&m_timer, SIGNAL(timeout()),
	           this,     SLOT(updateElapsedTime()));

	m_timer.stop();
}

//-----------------------------------------------------------------
PomodoroStatistics::Result PomodoroStatistics::getResult() const
{
  return m_result;
}

//-----------------------------------------------------------------
void PomodoroStatistics::invalidate()
{
	m_pomodoro->invalidateCurrent();

	updateGUI();
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
	m_paused = !m_paused;
	m_pomodoro->pause();

	updateGUI();
}

//-----------------------------------------------------------------
void PomodoroStatistics::updateElapsedTime()
{
	if (m_pomodoro->status() == Pomodoro::Status::Stopped)
	{
		m_elapsedTime->setText("--:--:--");
	}
	else
	{
		auto elapsedTime = m_pomodoro->elapsedTime();
		m_elapsedTime->setText(elapsedTime.toString(Qt::TextDate));

		updateProgress();
	}

	m_elapsedTime->repaint();
}

void PomodoroStatistics::updateProgress() const
{
  auto time = QTime(0,0,0,0);

  auto totalSecs = time.secsTo(m_pomodoro->getPomodoroDuration()) * m_pomodoro->completedPomodoros();
  totalSecs += time.secsTo(m_pomodoro->getShortBreakDuration()) * m_pomodoro->completedShortBreaks();
  totalSecs += time.secsTo(m_pomodoro->getLongBreakDuration()) * m_pomodoro->completedLongBreaks();

  auto elapsedSecs = time.secsTo(m_pomodoro->elapsedTime());
  auto sessionSecs = time.secsTo(m_pomodoro->sessionTime());

  m_progress->setValue(((totalSecs+elapsedSecs)*100)/sessionSecs);
}

//-----------------------------------------------------------------
void PomodoroStatistics::updateGUI()
{
  if (m_paused)
  {
    m_pause->setText("Resume");
  }
  else
  {
    m_pause->setText("Pause");
  }

  m_invalidate->setEnabled(!m_paused);
  m_continue  ->setEnabled(!m_paused);
  m_stop      ->setEnabled(!m_paused);

	unsigned long totalSecs = 0;
	auto pomodoroTime = QTime(0, 0, 0, 0);

	unsigned long seconds = pomodoroTime.secsTo(m_pomodoro->getPomodoroDuration());
	seconds *= m_pomodoro->completedPomodoros();
	totalSecs += seconds;
	pomodoroTime = pomodoroTime.addSecs(seconds);

	m_completedPomodoros->setText(QString("%1 (%2)").arg(m_pomodoro->completedPomodoros()).arg(pomodoroTime.toString(Qt::TextDate)));

	auto shortBreakTime = QTime(0, 0, 0, 0);
	seconds = shortBreakTime.secsTo(m_pomodoro->getShortBreakDuration());
	seconds *= m_pomodoro->completedShortBreaks();
	totalSecs += seconds;
	shortBreakTime = shortBreakTime.addSecs(seconds);

	m_completedShortBreaks->setText(QString("%1 (%2)").arg(m_pomodoro->completedShortBreaks()).arg(shortBreakTime.toString(Qt::TextDate)));

	auto longBreakTime = QTime(0, 0, 0, 0);
	seconds = longBreakTime.secsTo(m_pomodoro->getLongBreakDuration());
	seconds *= m_pomodoro->completedLongBreaks();
	totalSecs += seconds;
	longBreakTime = longBreakTime.addSecs(seconds);

	m_completedLongBreaks->setText(QString("%1 (%2)").arg(m_pomodoro->completedLongBreaks()).arg(longBreakTime.toString(Qt::TextDate)));

	auto totalTime = QTime(0, 0, 0, 0);
	totalTime = totalTime.addSecs(totalSecs);

	auto elapsedTime = m_pomodoro->elapsedTime();
	m_elapsedTime->setText(elapsedTime.toString(Qt::TextDate));

	updateProgress();

	switch(m_pomodoro->status())
	{
	  case Pomodoro::Status::LongBreak:
	    m_status->setText("<span style=\" color:#00ff00;\">In a long break</span>");
	    break;
	  case Pomodoro::Status::ShortBreak:
			m_status->setText("<span style=\" color:#0000ff;\">In a short break</span>");
	    break;
	  case Pomodoro::Status::Pomodoro:
	    m_status->setText("<span style=\" color:#ff0000;\">In a pomodoro</span>");
	    break;
	  case Pomodoro::Status::Paused:
			m_status->setText("<span style=\" color:#000000;\">Paused</span>");
	    break;
	  case Pomodoro::Status::Stopped:
      m_status->setText("<span style=\" color:#000000;\">Session finished</span>");
      m_elapsedTime->setText("--:--:--");
      m_invalidate->setEnabled(false);
      m_pause->setEnabled(false);
	    break;
	  default:
	    Q_ASSERT(false);
	    break;
	}

	m_completedTime->setText(totalTime.toString(Qt::TextDate));

	m_taskName->setText(m_pomodoro->getTask());

	repaint();
}

//-----------------------------------------------------------------
void PomodoroStatistics::updateTask()
{
	 bool ok;
	 auto text = QInputDialog::getText(this,
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
