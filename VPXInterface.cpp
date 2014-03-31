/*
 * VP8Interface.cpp
 *
 *  Created on: 07/03/2014
 *      Author: Felix de las Pozas Alvarez
 */

// Project
#include "VPXInterface.h"
#include "webmEBMLwriter.h"

// libyuv
#include "libyuv/convert.h"

// Qt
#include <QImage>
#include <QDebug>

#define fourcc    0x30385056
#define interface (vpx_codec_vp8_cx())

const int VPX_Interface::VP8_quality_values[3]{ VPX_DL_REALTIME, VPX_DL_GOOD_QUALITY, VPX_DL_BEST_QUALITY };

//------------------------------------------------------------------
VPX_Interface::VPX_Interface(QString fileName, int height, int width, int quality)
: m_vp8_filename{fileName}
, m_width{width - (width % 16)}
, m_height{height - (height % 16)}
, m_quality{VP8_quality_values[quality]}
, m_hash{0}
{
	// open output file
	if(!(m_ebml.file = fopen(m_vp8_filename.toStdString().c_str(), "w")))
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

	// populate encoder configuration
	auto res = vpx_codec_enc_config_default(interface, &m_vp8_cfg, 0);
	if (VPX_CODEC_OK != res)
	{
		qDebug() << QString("Failed to get config: %1").arg(vpx_codec_err_to_string(res));
		return;
	}

	m_vp8_cfg.g_w = m_width;
	m_vp8_cfg.g_h = m_height;
	m_vp8_cfg.rc_target_bitrate = m_width * m_height * m_vp8_cfg.rc_target_bitrate / m_vp8_cfg.g_w / m_vp8_cfg.g_h;
	m_vp8_cfg.g_timebase.num = 1001;
	m_vp8_cfg.g_timebase.den = 30000;
	m_vp8_cfg.g_threads = 2;
	m_vp8_cfg.g_pass = VPX_RC_ONE_PASS;
	m_vp8_cfg.g_profile = 0;            // Default profile.
  m_vp8_cfg.rc_min_quantizer = 0;
  m_vp8_cfg.rc_max_quantizer = 63;    // Maximum possible range.
  m_vp8_cfg.kf_mode = VPX_KF_AUTO;    // Auto key frames.

	// Initialize codec
	if (vpx_codec_enc_init(&m_vp8_codec, interface, &m_vp8_cfg, 0))
	{
		qDebug() << "Failed to initialize encoder";
		return;
	}

	writeWebmHeader();
}

//------------------------------------------------------------------
VPX_Interface::~VPX_Interface()
{
	vpx_img_free(&m_vp8_rawImage);
	if (vpx_codec_destroy(&m_vp8_codec))
	{
		qDebug() << "Failed to destroy codec";
	}

	writeWebmFooter();
	fclose(m_ebml.file);
}

//------------------------------------------------------------------
void VPX_Interface::encodeFrame(QImage* frame, unsigned long frameNumber)
{
	libyuv::ARGBToI420(frame->bits(), m_width * 4,
	                   m_vp8_rawImage.planes[0], m_vp8_rawImage.stride[0],
	                   m_vp8_rawImage.planes[1], m_vp8_rawImage.stride[1],
	                   m_vp8_rawImage.planes[2], m_vp8_rawImage.stride[2],
	                   m_width, m_height);

//	QString frameName = QString("D:\\Descargas\\rawFrame") + QString::number(frameNumber) + QString(".raw");
//	FILE *rawFrame = fopen(frameName.toStdString().c_str(), "w");
//	if (rawFrame != nullptr)
//	{
//		unsigned char *temp = m_vp8_rawImage.img_data;
//		int bytes = fwrite(temp, m_height * m_width * 3, 1, rawFrame);
//		fclose(rawFrame);
//	}

	vpx_codec_iter_t iter = nullptr;
	const vpx_codec_cx_pkt_t *pkt = nullptr;

	int result = vpx_codec_encode(&m_vp8_codec, &m_vp8_rawImage, frameNumber, 1, 0, m_quality);
	if (VPX_CODEC_OK != result)
	{
		qDebug() << "Failed to encode frame" << frameNumber;
		switch(result)
		{
			case VPX_CODEC_OK: qDebug() << "??"; break;
			case VPX_CODEC_INCAPABLE: qDebug() << "codec incapable"; break;
			case VPX_CODEC_INVALID_PARAM: qDebug() << "invalid param" << QString(vpx_codec_error_detail(&m_vp8_codec));

			break;
		}
	}

	while ((pkt = vpx_codec_get_cx_data(&m_vp8_codec, &iter)))
	{
		if (pkt->kind == VPX_CODEC_CX_FRAME_PKT)
		{
				m_hash = murmur(pkt->data.frame.buf, (int)pkt->data.frame.sz, m_hash);
				writeWebmBlock(pkt);
			  qDebug() << "frame" << frameNumber << (pkt->kind == VPX_CODEC_CX_FRAME_PKT && (pkt->data.frame.flags & VPX_FRAME_IS_KEY) ? "K" : ".") << "packet size" << pkt->data.frame.sz;
		}
	}
}

//------------------------------------------------------------------
bool VPX_Interface::writeWebmHeader()
{
  return true;
}

//------------------------------------------------------------------
void VPX_Interface::writeWebmBlock(const vpx_codec_cx_pkt_t* packet)
{
}

//------------------------------------------------------------------
bool VPX_Interface::writeWebmFooter()
{
	return true;
}
