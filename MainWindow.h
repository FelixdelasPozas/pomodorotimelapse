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
#include "ui_MainWindow.h"

// Qt
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMutex>

class QAction;
class QMenu;
class QPlainTextEdit;
class QProgressDialog;
class QResizeEvent;
class QEvent;
class QPixmap;

class CaptureDesktopThread;

class MainWindow
: public QMainWindow
, public Ui_MainWindow
{
	Q_OBJECT

	public:
		MainWindow();
		~MainWindow();

		void changeEvent(QEvent*);

	private slots:
	  void updateMonitorsComboBox(int status);
	  void updateMonitorsCheckBox(int status);
	  void updateCameraResolutionsComboBox(int status);
	  void activateTrayIcon(QSystemTrayIcon::ActivationReason);
	  void renderImage();

	private:
	  void setupCameraResolutions();
	  void setupMonitors();
	  void setupTrayIcon();
	  void setupCaptureThread();
	  void saveCapture();

		QStringList           m_cameraResolutionsNames;
		ResolutionList        m_cameraResolutions;
		Pomodoro              m_pomodoro;
		QPixmap               m_desktopCapture;
		QSystemTrayIcon      *m_trayIcon;
		CaptureDesktopThread *m_captureThread;
		QMutex                m_mutex;
		QPixmap              *m_cameraPixmap;

};

#endif /* MAINWINDOW_H_ */
