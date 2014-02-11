/*
 * MainWindow.h
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <QMainWindow>
#include "ui_MainWindow.h"

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

	private slots:
		void open();
		void about();
		void parserSelected(bool);

	private:
		void loadFile(const QString &fileName);
		bool saveFile(const QString &fileName);

		QString m_filename;
};

#endif /* MAINWINDOW_H_ */
