/*
 * MainWindow.h
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

// Qt
#include <QMainWindow>
#include "ui_MainWindow.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

class QAction;
class QMenu;
class QPlainTextEdit;

class MainWindow
: public QMainWindow
, public Ui_MainWindow
{
	Q_OBJECT

	public:
		MainWindow();
		~MainWindow();

	private slots:
	  void updateMonitorsComboBox(int status);
	  void updateCameraResolutionsComboBox(int status);

	private:
	  void setupMonitors();
	  void setupCameraResolutions();

	  cv::VideoCapture cap;
};

#endif /* MAINWINDOW_H_ */
