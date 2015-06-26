/*
    File: PomodoroStatistics.h
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

#ifndef POMODORO_STATISTICS_H_
#define POMODORO_STATISTICS_H_

// Project
#include "Pomodoro.h"
#include "ui_PomodoroStatistics.h"

// Qt
#include <QDialog>
#include <QTimeLine>
#include <QTimer>

// C++
#include <memory>

class QObject;
class QEvent;

/** \class PomodoroStatistics
 *  \brief Pomodoro statistics and control dialog.
 *
 */
class PomodoroStatistics
: public QDialog
, public Ui_PomodoroStatistics
{
	Q_OBJECT
	public:
	  /** \brief PomodoroStatistics class constructor.
	   * \param[in] pomodoro pomodoro smart pointer.
	   * \param[in] parent raw pointer of the parent of this object.
	   *
	   */
		explicit PomodoroStatistics(std::shared_ptr<Pomodoro> pomodoro, QWidget *parent = nullptr);

		/** \brief PomodoroStatistics class virtual destructor.
		 *
		 */
		virtual ~PomodoroStatistics();

		enum class Result : char { None = 1, Stop = 2, Continue = 3};

		/** \brief Returns the result of the dialog after closing.
		 *
		 */
		Result getResult() const;

	private slots:
    /** \brief Updates the dialog with the pomodoro values.
     *
     */
    void updateGUI();

    /** \brief Invalidates current pomodoro unit.
     *
     */
		void invalidate();

		/** \brief Stops the pomodoro and quits the dialog.
		 *
		 */
		void stop();

		/** \brief Closes the dialog.
		 *
		 */
		void resume();

		/** \brief Pauses the pomodoro.
		 *
		 */
		void pause();

		/** \brief Updates the elapsed time in the dialog.
		 *
		 */
		void updateElapsedTime();

		/** \brief Updates the pomodoro's task.
		 *
		 */
		void updateTask();

	private:
		void updateProgress() const;

		std::shared_ptr<Pomodoro> m_pomodoro;
		QTimeLine                 m_timeLine;
		QTimer                    m_timer;
		bool                      m_paused;
		Result                    m_result;
};

#endif // POMODORO_STATISTICS_H_
