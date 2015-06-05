/*
    File: VPxInterface.h
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

/** \class VPX_Interface
 *  \brief Interface to the VP8-VP9 library.
 *
 */
class VPX_Interface
{
	public:
    /** \brief VPX_Interface class constructor.
     * \param[in] fileName name of the video file to write.
     * \param[in] height height of the video in pixels.
     * \param[in] width width of the video in pixels.
     * \param[in] fps desired frames per second of the video.
     * \param[in] scaleRatio scale ratio from the initial size, value [0.5-2.0] default 1.0 (no rescaling)
     *
     */
		VPX_Interface(const QString fileName, const int height, const int width, const int fps, const float scaleRatio = 1.0);

		/** \brief VPX_Interface class virtual destructor.
		 *
		 */
		virtual ~VPX_Interface();

		/** \brief Encodes a frame to the video stream.
		 * \param[in] frame raw pointer of the frame to encode.
		 *
		 */
		void encodeFrame(QImage *frame);

	private:
		static const int VP8_quality_values[3];

		/** \brief Returns true if the image needs to be rescaled.
		 *
		 */
		bool scalingEnabled() const;

		vpx_image_t           m_vp8_rawImage;       /** vp8 frame image.                                  */
		vpx_image_t           m_vp8_rawImageScaled; /** vp8 frame image scaled.                           */
		vpx_codec_enc_cfg_t   m_vp8_config;         /** codec configuration                               */
		vpx_codec_ctx_t       m_vp8_context;        /** codec context                                     */
		QString               m_vp8_filename;       /** video file name                                   */
		int                   m_width;              /** video resolution width                            */
		int                   m_height;             /** video resolution height                           */
		float                 m_scale;              /** ratio of scale between scaled image and original. */
		int                   m_quality;            /** quality of the video                              */
		int                   m_hash;               /** murmur hash                                       */
		long int              m_frameNumber;        /** number of the current frame.                      */
		int                   m_fps;                /** video's frames per second                         */

		EbmlGlobal            m_ebml;               /** ebml structure (matroska's)                       */
};

#endif /* VPX_INTERFACE_H_ */
