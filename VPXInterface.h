/*
 * VPxInterface.h
 *
 *  Created on: 07/03/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef VPX_INTERFACE_H_
#define VPX_INTERFACE_H_

// VPX
#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vpx_codec.h"
#include "vpx/vpx_image.h"

// Project
#include "webmEBMLwriter.h"

// C++
#include <stdio.h>

// Qt
#include <QString>
#include <QStack>
class QImage;

class VPX_Interface
{
	public:
		VPX_Interface(QString fileName, int height, int width, int fps);
		virtual ~VPX_Interface();

		void encodeFrame(QImage *frame);

	private:
		static const int VP8_quality_values[3];

		vpx_image_t           m_vp8_rawImage;
//		vpx_image_t           m_vp8_rawImageScaled;
		vpx_codec_enc_cfg_t   m_vp8_config;
		vpx_codec_ctx_t       m_vp8_context;
		QString               m_vp8_filename;
		int                   m_width;
		int                   m_height;
		int                   m_quality;
		int                   m_hash;
		long int              m_frameNumber;
		int                   m_fps;

		EbmlGlobal            m_ebml;
};

#endif // VPX_INTERFACE_H_
