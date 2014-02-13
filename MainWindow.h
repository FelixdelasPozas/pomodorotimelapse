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

// Qt
#include <QMainWindow>
#include "ui_MainWindow.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

class QAction;
class QMenu;
class QPlainTextEdit;
class QProgressDialog;

class MainWindow
: public QMainWindow
, public Ui_MainWindow
{
	Q_OBJECT

	public:
		MainWindow();
		~MainWindow();

  signals:
    void setDialogText(QString);
    void progressValueChanged(int);

	private slots:
	  void updateMonitorsComboBox(int status);
	  void updateCameraResolutionsComboBox(int status);

	private:
	  static void probeResolutions(MainWindow *, QProgressDialog *);
	  void setupMonitors();
	  void setupCameraResolutions();

	  cv::VideoCapture cap;
		QStringList m_cameraResolutionsNames;
		ResolutionList m_cameraResolutions;

};

#endif /* MAINWINDOW_H_ */
