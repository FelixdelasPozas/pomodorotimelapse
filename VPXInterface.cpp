/*
 * VP8Interface.cpp
 *
 *  Created on: 07/03/2014
 *      Author: Felix de las Pozas Alvarez
 */

// Project
#include "VPXInterface.h"

// libyuv
#include "libyuv/convert.h"
//#include "libyuv/scale.h"

// Qt
#include <QImage>
#include <QDebug>

const int VPX_Interface::VP8_quality_values[3]{ VPX_DL_REALTIME, VPX_DL_GOOD_QUALITY, VPX_DL_BEST_QUALITY };

//------------------------------------------------------------------
VPX_Interface::VPX_Interface(QString fileName, int height, int width, int quality)
: m_vp8_filename{fileName}
, m_width{width - (width % 16)}
, m_height{height - (height % 16)}
, m_quality{VP8_quality_values[quality]}
, m_hash{0}
, m_frameNumber{0}
{
	memset(&m_ebml, 0, sizeof(EbmlGlobal));
	m_ebml.last_pts_ms = -1;

	// open output file
	if(!(m_ebml.stream = fopen(m_vp8_filename.toStdString().c_str(), "wb")))
	{
		qDebug() << "failed to open file" << m_vp8_filename;
		return;
	}

	// create buffer for frames
	if (!vpx_img_alloc(&m_vp8_rawImage, VPX_IMG_FMT_I420, m_width, m_height, 1))
	{
		qDebug() << "cannot allocate memory for image";
		return;
	}

//	if (!vpx_img_alloc(&m_vp8_rawImageScaled, VPX_IMG_FMT_I420, 720, 304, 1))
//	{
//		qDebug() << "cannot allocate memory for image";
//		return;
//	}


	// populate encoder configuration, we won't use VP9 because the 1-pass is
	// not yet up to the task
	auto res = vpx_codec_enc_config_default(vpx_codec_vp8_cx(), &m_vp8_config, 0);
	if (VPX_CODEC_OK != res)
	{
		qDebug() << QString("Failed to get config: %1").arg(vpx_codec_err_to_string(res));
		return;
	}

	m_vp8_config.rc_target_bitrate = m_width * m_height * m_vp8_config.rc_target_bitrate / m_vp8_config.g_w / m_vp8_config.g_h;
	m_vp8_config.g_w = m_width;
	m_vp8_config.g_h = m_height;
	m_vp8_config.g_timebase.num = 1;
	m_vp8_config.g_timebase.den = 30;
	m_vp8_config.g_threads = 2;
	m_vp8_config.g_pass = VPX_RC_ONE_PASS;
	m_vp8_config.g_profile = 0;            // Default profile.
  m_vp8_config.rc_min_quantizer = 0;
  m_vp8_config.rc_max_quantizer = 63;    // Maximum possible range.
  m_vp8_config.kf_mode = VPX_KF_AUTO;    // Auto key frames.

  m_ebml.framerate = m_vp8_config.g_timebase;

	// Initialize codec
	if (vpx_codec_enc_init(&m_vp8_context, vpx_codec_vp8_cx(), &m_vp8_config, 0))
	{
		qDebug() << "Failed to initialize encoder";
		return;
	}

	struct vpx_rational arg_framerate = {30, 1};
	write_webm_file_header(&m_ebml, &m_vp8_config, &arg_framerate);
}

//------------------------------------------------------------------
VPX_Interface::~VPX_Interface()
{
	vpx_img_free(&m_vp8_rawImage);
	if (vpx_codec_destroy(&m_vp8_context))
	{
		qDebug() << "Failed to destroy codec";
	}

	write_webm_file_footer(&m_ebml, m_hash);
	fclose(m_ebml.stream);
}

//------------------------------------------------------------------
void VPX_Interface::encodeFrame(QImage* frame)
{
	++m_frameNumber;

	libyuv::ARGBToI420(frame->bits(), m_width * 4,
	                   m_vp8_rawImage.planes[0], m_vp8_rawImage.stride[0],
	                   m_vp8_rawImage.planes[1], m_vp8_rawImage.stride[1],
	                   m_vp8_rawImage.planes[2], m_vp8_rawImage.stride[2],
	                   m_width, m_height);

//	int result2 = libyuv::I420Scale(m_vp8_rawImage.planes[0], m_vp8_rawImage.stride[0],
//      	            m_vp8_rawImage.planes[1], m_vp8_rawImage.stride[1],
//      	            m_vp8_rawImage.planes[2], m_vp8_rawImage.stride[2],
//      	            m_width, m_height,
//      	            m_vp8_rawImageScaled.planes[0], m_vp8_rawImageScaled.stride[0],
//      	            m_vp8_rawImageScaled.planes[1], m_vp8_rawImageScaled.stride[1],
//      	            m_vp8_rawImageScaled.planes[2], m_vp8_rawImageScaled.stride[2],
//      	            m_width/2, m_height/2,
//      	            libyuv::kFilterBox);
//
//	qDebug() << "new witdh" << m_width/2 << "new height" << m_height/2 << "failed?" << (result2 != 0);

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

	int result = vpx_codec_encode(&m_vp8_context, &m_vp8_rawImage, m_frameNumber, 33, 0, m_quality);
	if (VPX_CODEC_OK != result)
	{
		qDebug() << "Failed to encode frame" << m_frameNumber;
		switch(result)
		{
			case VPX_CODEC_OK: qDebug() << "??"; break;
			case VPX_CODEC_INCAPABLE: qDebug() << "codec incapable"; break;
			case VPX_CODEC_INVALID_PARAM: qDebug() << "invalid param" << QString(vpx_codec_error_detail(&m_vp8_context));

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

