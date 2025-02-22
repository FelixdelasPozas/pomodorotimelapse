/*
 File: AboutDialog.cpp
 Created on: 13/5/2015
 Author: Felix

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
#include <AboutDialog.h>

// dlib
#include <dlib/revision.h>

// OpenCV
#include <opencv2/core/utility.hpp>

// VPX
#include <vpx/vpx_codec.h>

// libyuv
#include <libyuv/version.h>

// Qt
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>

const QString AboutDialog::VERSION = QString("version 1.3.0");

//-----------------------------------------------------------------
AboutDialog::AboutDialog(QWidget *parent, Qt::WindowFlags f)
: QDialog(parent, f)
{
  setupUi(this);
  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & ~(Qt::WindowMaximizeButtonHint) & ~(Qt::WindowMinimizeButtonHint));

  const auto compilation_date = QString(__DATE__);
  const auto compilation_time = QString(" (") + QString(__TIME__) + QString(")");

  m_compilationDate->setText(tr("Compiled on ") + compilation_date + compilation_time);
  m_version->setText(VERSION);
  m_qtVersion->setText(tr("version %1").arg(qVersion()));
  m_dlibVersion->setText(tr("version %1.%2.%3").arg(DLIB_MAJOR_VERSION).arg(DLIB_MINOR_VERSION).arg(DLIB_PATCH_VERSION));
  m_opencvVersion->setText(tr("version %1").arg(QString::fromStdString(cv::getVersionString())));
  m_vpxVersion->setText(tr("version %1.%2.%3").arg(vpx_codec_version_major()).arg(vpx_codec_version_minor()).arg(vpx_codec_version_patch()));
  m_yuvVersion->setText(tr("version %1").arg(LIBYUV_VERSION));
  m_copy->setText(tr("Copyright (c) 2015-%1 Félix de las Pozas Álvarez").arg(QDateTime::currentDateTime().date().year()));

  QObject::connect(m_kofiLabel, &ClickableHoverLabel::clicked,
                  [this](){ QDesktopServices::openUrl(QUrl{"https://ko-fi.com/felixdelaspozas"}); });    
}

