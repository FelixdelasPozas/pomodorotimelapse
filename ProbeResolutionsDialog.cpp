/*
 * ProbeResolutionsDialog.cpp
 *
 *  Created on: 13/02/2014
 *      Author: Felix de las Pozas Alvarez
 */

#include "ProbeResolutionsDialog.h"
#include <QDebug>
//-----------------------------------------------------------------
ProbeResolutionsDialog::ProbeResolutionsDialog(QWidget *parent)
: QDialog(parent)
{
	setupUi(this);
	m_progressBar->setValue(0);

	m_thread = new ProbeThread(this);
	connect(m_thread, SIGNAL(probed(QString)), this, SLOT(updateLabel(QString)));
	connect(m_thread, SIGNAL(progress(int)), this, SLOT(updateProgress(int)));
	connect(m_thread, SIGNAL(finished()), this, SLOT(threadFinished()));
	connect(this, SIGNAL(rejected()), this, SLOT(cancelThread()));

	m_thread->start();
}

//-----------------------------------------------------------------
void ProbeResolutionsDialog::threadFinished()
{
	if(!m_thread->isCancelled())
		m_availableResolutions = m_thread->getResolutions();

	done(QDialog::Accepted);
}

//-----------------------------------------------------------------
ProbeResolutionsDialog::~ProbeResolutionsDialog()
{
	m_thread->wait();
	delete m_thread;
}

//-----------------------------------------------------------------
ProbeThread::ProbeThread(QObject *parent)
: QThread(parent)
, m_cancelled{false}
{
}

//-----------------------------------------------------------------
ProbeThread::~ProbeThread()
{
	if (m_camera.isOpened())
		m_camera.release();
}

//-----------------------------------------------------------------
void ProbeThread::run()
{
	if (!m_camera.open(0))
		return;

	for (Resolution resolution : CommonResolutions)
	{
		if (m_cancelled)
			return;

		emit probed(QString("%1x%2 (").arg(resolution.width).arg(resolution.height) + resolution.name + QString(" resolution)"));
		emit progress((CommonResolutions.indexOf(resolution)+1)*100/CommonResolutions.size());

		m_camera.set(CV_CAP_PROP_FRAME_WIDTH, resolution.width);
		m_camera.set(CV_CAP_PROP_FRAME_HEIGHT, resolution.height);
		auto width = m_camera.get(CV_CAP_PROP_FRAME_WIDTH);
		auto height = m_camera.get(CV_CAP_PROP_FRAME_HEIGHT);

		if ((width == resolution.width) && (height == resolution.height))
			m_availableResolutions << resolution;
	}
}
