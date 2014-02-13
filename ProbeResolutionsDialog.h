/*
 * ProbeResolutionsDialog.h
 *
 *  Created on: 13/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef PROBERESOLUTIONSDIALOG_H_
#define PROBERESOLUTIONSDIALOG_H_

// Application
#include "Resolutions.h"
#include "ui_ProbeResolutionsDialog.h"

// Qt
#include <QDialog>
#include <QThread>

// OpenCV
#include <opencv2/highgui/highgui.hpp>

class ProbeThread
: public QThread
{
	Q_OBJECT
	public:
		explicit ProbeThread(QObject *parent = nullptr);
		virtual ~ProbeThread();

		void run();

		void cancel()
		{ m_cancelled = true; }

		bool isCancelled()
		{ return m_cancelled; }

		ResolutionList getResolutions()
		{ return m_availableResolutions; };

  signals:
		void probed(QString resolutionName);
		void progress(int progress);

	private:
		cv::VideoCapture m_camera;
		bool             m_cancelled;
		ResolutionList   m_availableResolutions;


};

class ProbeResolutionsDialog
: public QDialog
, public Ui_ProbeDialog
{
	Q_OBJECT
	public:
		explicit ProbeResolutionsDialog(QWidget *parent = nullptr);
		virtual ~ProbeResolutionsDialog();

		ResolutionList getResolutions()
		{ return m_availableResolutions; };

	public slots:
		void threadFinished();
		void updateLabel(QString label)
		{ m_resolutionLabel->setText(label); };

		void updateProgress(int progress)
		{ m_progressBar->setValue(progress); };

		void cancelThread()
		{ m_thread->cancel(); };

	private:
		ProbeThread    *m_thread;
		ResolutionList  m_availableResolutions;
};



#endif /* PROBERESOLUTIONSDIALOG_H_ */
