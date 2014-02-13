/*
 * MainWindow.h
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

// Application
#include "Resolutions.h"
#include "Pomodoro.h"

// Qt
#include <QMainWindow>
#include "ui_MainWindow.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

class QAction;
class QMenu;
class QPlainTextEdit;
class QProgressDialog;
class QResizeEvent;

class MainWindow
: public QMainWindow
, public Ui_MainWindow
{
	Q_OBJECT

	public:
		MainWindow();
		~MainWindow();

		void resizeEvent(QResizeEvent *);

	private slots:
	  void updateMonitorsComboBox(int status);
	  void updateCameraResolutionsComboBox(int status);

	private:
	  void setupMonitors();
	  void setupCameraResolutions();
	  void updateCapturedImage();
	  void saveCapture();

	  cv::VideoCapture m_camera;
		QStringList      m_cameraResolutionsNames;
		ResolutionList   m_cameraResolutions;
		Pomodoro         m_pomodoro;
		QPixmap          m_desktopCapture;

};

#endif /* MAINWINDOW_H_ */
