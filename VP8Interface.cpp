/*
 * VP8Interface.cpp
 *
 *  Created on: 07/03/2014
 *      Author: Felix de las Pozas Alvarez
 */

// Project
#include "VP8Interface.h"
#include "webmEBMLwriter.h"
#include "RGB2YUV.h"

// Qt
#include <QImage>
#include <QDebug>

#define fourcc    0x30385056
#define interface (vpx_codec_vp8_cx())
#define IVF_FILE_HDR_SZ  (32)
#define IVF_FRAME_HDR_SZ (12)

const int VP8_Interface::VP8_quality_values[3]{ VPX_DL_REALTIME, VPX_DL_GOOD_QUALITY, VPX_DL_BEST_QUALITY };

//------------------------------------------------------------------
VP8_Interface::VP8_Interface(QString fileName, int height, int width, int quality)
: m_vp8_filename{fileName}
, m_width{width}
, m_height{height}
, m_quality{VP8_quality_values[quality]}
, m_hash{0}
{
	// open output file
	if(!(m_state.stream = fopen(m_vp8_filename.toStdString().c_str(), "w")))
	{
		qDebug() << "failed to open file" << m_vp8_filename;
		return;
	}
	m_state.last_pts_ms = -1;

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
	m_vp8_cfg.rc_target_bitrate = m_width * m_height * m_vp8_cfg.rc_target_bitrate / m_vp8_cfg.g_w / m_vp8_cfg.g_h;
	m_vp8_cfg.g_w = m_width;
	m_vp8_cfg.g_h = m_height;
	m_vp8_cfg.g_timebase.num = 1001;
	m_vp8_cfg.g_timebase.den = 30000;

	// Initialize codec
	if (vpx_codec_enc_init(&m_vp8_codec, interface, &m_vp8_cfg, 0))
	{
		qDebug() << "Failed to initialize encoder";
		return;
	}

	write_webm_file_header(&m_state, &m_vp8_cfg, &m_vp8_cfg.g_timebase, STEREO_FORMAT_MONO, static_cast<unsigned int>(fourcc));
}

//------------------------------------------------------------------
VP8_Interface::~VP8_Interface()
{
	vpx_img_free(&m_vp8_rawImage);
	if (vpx_codec_destroy(&m_vp8_codec))
	{
		qDebug() << "Failed to destroy codec";
	}

	write_webm_file_footer(&m_state, m_hash);
	fclose(m_state.stream);
}

//------------------------------------------------------------------
void VP8_Interface::encodeFrame(QImage* frame, unsigned long frameNumber)
{
	rgb32_to_i420(m_width, m_height, frame->bits(), m_vp8_rawImage.img_data);

	vpx_codec_iter_t iter = nullptr;
	const vpx_codec_cx_pkt_t *pkt;

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
		switch (pkt->kind)
		{
			case VPX_CODEC_CX_FRAME_PKT:
				m_hash = murmur(pkt->data.frame.buf, (int)pkt->data.frame.sz, m_hash);
			  write_webm_block(&m_state, &m_vp8_cfg, pkt);
			  qDebug() << "frame" << frameNumber << (pkt->kind == VPX_CODEC_CX_FRAME_PKT && (pkt->data.frame.flags & VPX_FRAME_IS_KEY) ? "K" : ".") << "packet size" << pkt->data.frame.sz;
				break;
			default:
				break;
		}
	}
}
