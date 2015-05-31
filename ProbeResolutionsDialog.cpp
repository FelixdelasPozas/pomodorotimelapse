/*
    File: ProbeResolutionsDialog.cpp
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

// Project
#include "ProbeResolutionsDialog.h"

//-----------------------------------------------------------------
ProbeResolutionsDialog::ProbeResolutionsDialog(QWidget *parent)
: QDialog{parent}
{
	setupUi(this);
	m_progressBar->setValue(0);

	m_thread = new ProbeThread(this);

	connect(m_thread, SIGNAL(probed(QString)),
	        this,     SLOT(updateLabel(QString)));

	connect(m_thread, SIGNAL(progress(int)),
	        this,     SLOT(updateProgress(int)));

	connect(m_thread, SIGNAL(finished()),
	        this,     SLOT(threadFinished()));

	connect(this, SIGNAL(rejected()),
	        this, SLOT(cancelThread()));

	m_thread->start();
}

//-----------------------------------------------------------------
ProbeResolutionsDialog::~ProbeResolutionsDialog()
{
	m_thread->wait();
	delete m_thread;
}

//-----------------------------------------------------------------
ResolutionList ProbeResolutionsDialog::getResolutions() const
{
  return m_availableResolutions;
}

//-----------------------------------------------------------------
void ProbeResolutionsDialog::threadFinished()
{
	if(!m_thread->hasBeenStopped())
	{
		m_availableResolutions = m_thread->getResolutions();
	}

	done(QDialog::Accepted);
}

//-----------------------------------------------------------------
void ProbeResolutionsDialog::updateLabel(QString label)
{
  m_resolutionLabel->setText(label);
}

//-----------------------------------------------------------------
void ProbeResolutionsDialog::updateProgress(int progress)
{
  m_progressBar->setValue(progress);
}

//-----------------------------------------------------------------
void ProbeResolutionsDialog::cancelThread()
{
  m_thread->stop();
}

//-----------------------------------------------------------------
ProbeThread::ProbeThread(QObject *parent)
: QThread{parent}
, m_stop {false}
{
}

//-----------------------------------------------------------------
ProbeThread::~ProbeThread()
{
	if (m_camera.isOpened())
	{
		m_camera.release();
	}
}

//-----------------------------------------------------------------
void ProbeThread::stop()
{
  m_stop = true;
}

//-----------------------------------------------------------------
bool ProbeThread::hasBeenStopped() const
{
  return m_stop;
}

//-----------------------------------------------------------------
ResolutionList ProbeThread::getResolutions() const
{
  return m_availableResolutions;
};

//-----------------------------------------------------------------
void ProbeThread::run()
{
	if (!m_camera.open(0)) return;

	for (Resolution resolution : CommonResolutions)
	{
		if (m_stop) return;

		emit probed(QString("%1x%2 - ").arg(resolution.width).arg(resolution.height) + resolution.name);

		auto value = (CommonResolutions.indexOf(resolution)+1)*100/CommonResolutions.size();
		emit progress(value);

		m_camera.set(CV_CAP_PROP_FRAME_WIDTH, resolution.width);
		m_camera.set(CV_CAP_PROP_FRAME_HEIGHT, resolution.height);
		auto width = m_camera.get(CV_CAP_PROP_FRAME_WIDTH);
		auto height = m_camera.get(CV_CAP_PROP_FRAME_HEIGHT);

		if ((width == resolution.width) && (height == resolution.height))
		{
			m_availableResolutions << resolution;
		}
	}
}
