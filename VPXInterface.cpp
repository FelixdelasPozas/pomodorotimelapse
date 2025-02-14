/*
    File: VPXInterface.cpp
    Created on: 07/03/2014
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
#include <VPXInterface.h>

// libyuv
#include "libyuv/convert.h"
#include "libyuv/scale.h"

// Qt
#include <QImage>
#include <QDebug>
#include <QFile>

const int VPX_Interface::VP8_quality_values[3]{ VPX_DL_REALTIME, VPX_DL_GOOD_QUALITY, VPX_DL_BEST_QUALITY };

//------------------------------------------------------------------
VPX_Interface::VPX_Interface(const QString fileName, const int height, const int width, const int fps, const float scaleRatio)
: m_vp8_filename{fileName}
, m_width       {width - (width % 16)}
, m_height      {height - (height % 16)}
, m_scale       {scaleRatio}
, m_quality     {VPX_DL_BEST_QUALITY}
, m_hash        {0}
, m_frameNumber {0}
, m_fps         {fps}
{
  if(m_scale < 0.5) m_scale = 0.5;
  if(m_scale > 2.0) m_scale = 2.0;

	memset(&m_ebml, 0, sizeof(EbmlGlobal));
	m_ebml.last_pts_ms = -1;

	// open output file
	if(!(m_ebml.stream = fopen(m_vp8_filename.toStdString().c_str(), "wb")))
	{
		qDebug() << "failed to open file" << m_vp8_filename;
		return;
	}

	// populate encoder configuration, we won't use VP9 because the 1-pass is
	// not yet up to the task
	auto res = vpx_codec_enc_config_default(vpx_codec_vp8_cx(), &m_vp8_config, 0);
	if (VPX_CODEC_OK != res)
	{
		qDebug() << QString("Failed to get config: %1").arg(vpx_codec_err_to_string(res));
		return;
	}

	if(scalingEnabled())
	{
	  m_vp8_config.g_w = m_width * m_scale - (static_cast<int>(m_width * m_scale) % 16);
	  m_vp8_config.g_h = m_height * m_scale - (static_cast<int>(m_height * m_scale) % 16);
	}
	else
	{
	  m_vp8_config.g_w = m_width;
	  m_vp8_config.g_h = m_height;
	}

  m_vp8_config.rc_target_bitrate = 12 * m_vp8_config.g_w * m_vp8_config.g_h / 1024;
	m_vp8_config.rc_dropframe_thresh = 0;
	m_vp8_config.rc_resize_allowed = 0;
	m_vp8_config.rc_end_usage = VPX_VBR;
  m_vp8_config.g_bit_depth = VPX_BITS_12;
	m_vp8_config.g_timebase.num = 1;
	m_vp8_config.g_timebase.den = m_fps;
	m_vp8_config.g_threads = 4;
	m_vp8_config.g_pass = VPX_RC_ONE_PASS;
	m_vp8_config.g_profile = 0;            // Default profile.
  m_vp8_config.rc_min_quantizer = 0;
  m_vp8_config.rc_max_quantizer = 63;    // 63 is maximum.
  m_vp8_config.kf_mode = VPX_KF_AUTO;    // Auto key frames.

  m_ebml.framerate = m_vp8_config.g_timebase;

	// Initialize codec
	if (vpx_codec_enc_init(&m_vp8_context, vpx_codec_vp8_cx(), &m_vp8_config, 0))
	{
		qDebug() << "Failed to initialize encoder";
		return;
	}

  // create buffer for frame
  if (!vpx_img_alloc(&m_vp8_rawImage, VPX_IMG_FMT_I420, m_width, m_height, 1))
  {
    qDebug() << "cannot allocate memory for image";
    return;
  }

  // create buffer for scaled frame
  if(scalingEnabled())
  {
    if (!vpx_img_alloc(&m_vp8_rawImageScaled, VPX_IMG_FMT_I420, m_vp8_config.g_w, m_vp8_config.g_h, 1))
    {
      qDebug() << "cannot allocate memory for image";
      return;
    }
  }

	struct vpx_rational framerate = {m_fps, 1};
	write_webm_file_header(&m_ebml, &m_vp8_config, &framerate);
}

//------------------------------------------------------------------
VPX_Interface::~VPX_Interface()
{
	vpx_img_free(&m_vp8_rawImage);
	if(scalingEnabled())
	  vpx_img_free(&m_vp8_rawImageScaled);

	if (vpx_codec_destroy(&m_vp8_context))
		qDebug() << "Failed to destroy codec";

	if (m_frameNumber != 0)
		write_webm_file_footer(&m_ebml, m_hash);
	else
		QFile::remove(m_vp8_filename);

	fclose(m_ebml.stream);
}

//------------------------------------------------------------------
void VPX_Interface::encodeFrame(QImage* frame)
{
	++m_frameNumber;

	vpx_image_t *image;

	libyuv::ARGBToI420(frame->bits(), m_width * 4,
	                   m_vp8_rawImage.planes[0], m_vp8_rawImage.stride[0],
	                   m_vp8_rawImage.planes[1], m_vp8_rawImage.stride[1],
	                   m_vp8_rawImage.planes[2], m_vp8_rawImage.stride[2],
	                   m_width, m_height);

	if(scalingEnabled())
	{
    libyuv::I420Scale(m_vp8_rawImage.planes[0], m_vp8_rawImage.stride[0],
                      m_vp8_rawImage.planes[1], m_vp8_rawImage.stride[1],
                      m_vp8_rawImage.planes[2], m_vp8_rawImage.stride[2],
                      m_width, m_height,
                      m_vp8_rawImageScaled.planes[0], m_vp8_rawImageScaled.stride[0],
                      m_vp8_rawImageScaled.planes[1], m_vp8_rawImageScaled.stride[1],
                      m_vp8_rawImageScaled.planes[2], m_vp8_rawImageScaled.stride[2],
                      m_vp8_config.g_w, m_vp8_config.g_h,
                      libyuv::kFilterBox);

    image = &m_vp8_rawImageScaled;
	}
	else
	  image = &m_vp8_rawImage;

// DUMP RAW FRAME ///////////////////////////////////////////////////////////////
//	QString frameName = QString("D:\\Descargas\\rawFrame") + QString::number(m_frameNumber) + QString(".raw");
//	FILE *rawFrame = fopen(frameName.toStdString().c_str(), "wb");
//	if (rawFrame != nullptr)
//	{
//		unsigned char *temp = m_vp8_rawImage.img_data;
//		fwrite(temp, (m_width * m_height * 3)/2, 1, rawFrame);
//		fclose(rawFrame);
//	}

	vpx_codec_iter_t iter = nullptr;
	const vpx_codec_cx_pkt_t *pkt;

	int result = vpx_codec_encode(&m_vp8_context, image, m_frameNumber, 1000/m_fps, 0, m_quality);
	if (VPX_CODEC_OK != result)
	{
		qDebug() << "Failed to encode frame" << m_frameNumber;
		switch(result)
		{
			case VPX_CODEC_INCAPABLE:
			  qDebug() << "codec incapable";
			  break;
			case VPX_CODEC_INVALID_PARAM:
			  qDebug() << "invalid param" << QString(vpx_codec_error_detail(&m_vp8_context));
			  break;
			default:
			  qDebug() << "unknown error";
			  break;
		}
	}

	while ((pkt = vpx_codec_get_cx_data(&m_vp8_context, &iter)))
	{
		if (pkt->kind == VPX_CODEC_CX_FRAME_PKT)
		{
				m_hash = murmur(pkt->data.frame.buf, (int)pkt->data.frame.sz, m_hash);
				write_webm_block(&m_ebml, &m_vp8_config, pkt);
		}
	}
}

//------------------------------------------------------------------
bool VPX_Interface::scalingEnabled() const
{
  return m_scale != 1.0;
}
