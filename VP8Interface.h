/*
 * VP8Interface.h
 *
 *  Created on: 07/03/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef VP8INTERFACE_H_
#define VP8INTERFACE_H_

// VPX
#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vpx_codec.h"
#include "vpx/vpx_image.h"

#include "webmEBMLwriter.h"

#include <QString>
class QImage;

class VP8_Interface
{
	public:
		VP8_Interface(QString fileName, int height, int width, int quality);
		~VP8_Interface();

		void encodeFrame(QImage *frame, unsigned long frameNumber);

	private:
		static const int VP8_quality_values[3];

		vpx_image_t           m_vp8_rawImage;
		vpx_codec_enc_cfg_t   m_vp8_cfg;
		vpx_codec_ctx_t       m_vp8_codec;
		QString               m_vp8_filename;
		int                   m_width;
		int                   m_height;
		int                   m_quality;
		int                   m_hash;

		EbmlGlobal            m_state;
};

#endif /* VP8INTERFACE_H_ */
