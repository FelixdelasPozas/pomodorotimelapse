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

// C++
#include <stdio.h>

// Qt
#include <QString>
#include <QStack>
class QImage;

struct EbmlGlobal
{
  FILE *file;
  vpx_rational fps;
  QStack<off_t> elementPos;

  EbmlGlobal(): file(nullptr), fps{1001, 30000} {};
};

class VPX_Interface
{
	public:
		VPX_Interface(QString fileName, int height, int width, int quality);
		~VPX_Interface();

		void encodeFrame(QImage *frame, unsigned long frameNumber);

	private:
		bool writeWebmHeader();
		void writeWebmBlock(const vpx_codec_cx_pkt_t* packet);
		bool writeWebmFooter();

		void StartSubElement(unsigned long class_id);
		void EndSubElement();
		void EbmlWrite(const void* buffer, unsigned long len);
		template<class T> void EbmlSerializeHelper(const T* buffer, unsigned long len);
		void EbmlSerialize(const void* buffer, int buffer_size, unsigned long len);

		static const int VP8_quality_values[3];

		vpx_image_t           m_vp8_rawImage;
		vpx_codec_enc_cfg_t   m_vp8_cfg;
		vpx_codec_ctx_t       m_vp8_codec;
		QString               m_vp8_filename;
		int                   m_width;
		int                   m_height;
		int                   m_quality;
		int                   m_hash;

		EbmlGlobal            m_ebml;
};


/* Murmur hash derived from public domain reference implementation at
 *   http://sites.google.com/site/murmurhash/
 */
static unsigned int murmur ( const void * key, int len, unsigned int seed )
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	unsigned int h = seed ^ len;
	const unsigned char * data = (const unsigned char *)key;

  while(len >= 4)
  {
  	unsigned int k;

  	k  = data[0];
  	k |= data[1] << 8;
  	k |= data[2] << 16;
  	k |= data[3] << 24;

  	k *= m;
  	k ^= k >> r;
  	k *= m;

  	h *= m;
  	h ^= k;

  	data += 4;
  	len -= 4;
  }

  switch(len)
  {
  	case 3: h ^= data[2] << 16;
  	case 2: h ^= data[1] << 8;
  	case 1: h ^= data[0];
  	h *= m;
  }

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

#endif // VPX_INTERFACE_H_
