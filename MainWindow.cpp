/*
 * MainWindow.cpp
 *
 *  Created on: 21/06/2013
 *      Author: Felix de las Pozas Alvarez
 */

// Project
#include "MainWindow.h"
#include "Resolutions.h"

// Qt
#include <QtGui>
#include <QDebug>


//-----------------------------------------------------------------
MainWindow::MainWindow()
{
	setupUi(this);

	this->setWindowTitle("Desktop Capture");
	this->setWindowIcon(QIcon(":/DesktopCapture/DesktopCapture.ico"));
	this->showMaximized();

	setupMonitors();
	setupCameraResolutions();
}

//-----------------------------------------------------------------
MainWindow::~MainWindow()
{
	if(cap.isOpened())
		cap.release();
}

//-----------------------------------------------------------------
void MainWindow::setupMonitors()
{
	QDesktopWidget *desktop = QApplication::desktop();
	QStringList monitors;

	for (int i = 0; i < desktop->numScreens(); ++i)
	{
		auto geometry = desktop->screenGeometry(i);
		if (desktop->primaryScreen() == i)
			monitors << QString("Primary Screen (Size: %1x%2 - Position: %3x%4)").arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
		else
			monitors << QString("Additional Screen %1 (Size: %2x%3 - Position: %4x%5)").arg(i).arg(geometry.width()).arg(geometry.height()).arg(geometry.x()).arg(geometry.y());
	}
	m_captureMonitorComboBox->insertItems(0, monitors);

	connect(m_captureAllMonitors, SIGNAL(stateChanged(int)), this, SLOT(updateMonitorsComboBox(int)));
}

//-----------------------------------------------------------------
void MainWindow::setupCameraResolutions()
{
	QDesktopWidget *desktop = QApplication::desktop();
	QStringList cameraResolutions;

	if (!cap.open(0))
	{
		m_enableCamera->setChecked(false);
		m_cameraResolutionComboBox->insertItem(0, QString("No cameras detected."));
		m_cameraResolutionComboBox->setEnabled(false);
		m_resolutionsScan->setEnabled(false);
	}
	else
	{
		QMessageBox resolutionsTry;
		resolutionsTry.setWindowIcon(QIcon(":/DesktopCapture/DesktopCapture.ico"));
		resolutionsTry.setVisible(true);
		resolutionsTry.setWindowTitle(QString("Checking camera resolutions"));
		resolutionsTry.setStandardButtons(0);

		QSpacerItem* horizontalSpacer = new QSpacerItem(300, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
		QGridLayout* layout = (QGridLayout*) resolutionsTry.layout();
		layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());

		resolutionsTry.repaint();
		resolutionsTry.move((desktop->screenGeometry(desktop->primaryScreen()).width() - 300) / 2,
				               ((desktop->screenGeometry(desktop->primaryScreen()).height() - resolutionsTry.height()) / 2));

		for (auto resolution : CommonResolutions)
		{
			resolutionsTry.setText(QString("Trying ") + checkResolution(resolution.width, resolution.height));
			resolutionsTry.repaint();

			cap.set(CV_CAP_PROP_FRAME_WIDTH, resolution.width);
			cap.set(CV_CAP_PROP_FRAME_HEIGHT, resolution.height);
			auto width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
			auto height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

			auto name = checkResolution(width, height);

			if (!cameraResolutions.contains(name))
				cameraResolutions << name;
		}

		m_cameraResolutionComboBox->insertItems(0, cameraResolutions);
		m_cameraResolutionComboBox->setCurrentIndex(cameraResolutions.size() / 2);

		connect(m_enableCamera, SIGNAL(stateChanged(int)), this, SLOT(updateCameraResolutionsComboBox(int)));
	}
}

//-----------------------------------------------------------------
void MainWindow::updateMonitorsComboBox(int status)
{
	m_captureMonitorComboBox->setEnabled(status == Qt::Unchecked);
}

//-----------------------------------------------------------------
void MainWindow::updateCameraResolutionsComboBox(int status)
{
	bool enabled = (status == Qt::Checked);
	m_cameraResolutionComboBox->setEnabled(enabled);
	m_resolutionsScan->setEnabled(enabled);
}

