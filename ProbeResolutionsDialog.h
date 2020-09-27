/*
    File: ProbeResolutionsDialog.h
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


#ifndef PROBERESOLUTIONSDIALOG_H_
#define PROBERESOLUTIONSDIALOG_H_

// Project
#include "Resolutions.h"
#include "ui_ProbeResolutionsDialog.h"

// Qt
#include <QDialog>
#include <QThread>

// OpenCV
#include <opencv2/highgui/highgui.hpp>

/** \class ProbeThread
 *  \brief Class for probing the available resolutions of the camera.
 *
 */
class ProbeThread
: public QThread
{
	Q_OBJECT
	public:
	  /** \brief ProbeThread class constructor.
	   * \param[in] parent raw pointer of the parent of this object.
	   *
	   */
		explicit ProbeThread(QObject *parent = nullptr);

		/** \brief ProbeThread class virtual destructor.
		 *
		 */
		virtual ~ProbeThread();

		virtual void run() final;

		/** \brief Stops the thread.
		 *
		 */
		void stop();

    /** \brief Returns true if the thread has been cancelled.
     *
     */
    bool hasBeenStopped() const;

		/** \brief Returns the list of valid resolutions for the camera.
		 *
		 */
		ResolutionList getResolutions() const;

  signals:
		void probed(QString resolutionName);
		void progress(int progress);

	private:
		cv::VideoCapture m_camera;
		bool             m_stop;
		ResolutionList   m_availableResolutions;
};

/** \class ProbeResolutionsDialog
 *  \brief Dialog that show the progress of the resolution checking of the camera.
 *
 */
class ProbeResolutionsDialog
: public QDialog
, public Ui_ProbeDialog
{
	Q_OBJECT
	public:
	  /** \brief ProbeResolutionsDialog class constructor.
	   * \param[in] parent raw pointer of the parent of this object.
	   * \param[in] f Window flags.
	   *
	   */
		explicit ProbeResolutionsDialog(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

		/** \brief ProbeResolutionsDialog class virtual destructor.
		 *
		 */
		virtual ~ProbeResolutionsDialog();

		/** \brief Returns the list of valid resolutions for the camera.
		 *
		 */
		ResolutionList getResolutions() const;

	public slots:
	  /** \brief
	   *
	   */
		void threadFinished();

    /** \brief Updates the dialog with the resolution being probed.
     *
     */
		void updateLabel(QString label);

    /** \brief Updates the progress of the dialog.
     *
     */
		void updateProgress(int progress);

    /** \brief Cancels the probing thread.
     *
     */
		void cancelThread();

	private:
		ProbeThread    *m_thread;
		ResolutionList  m_availableResolutions;
};



#endif /* PROBERESOLUTIONSDIALOG_H_ */
