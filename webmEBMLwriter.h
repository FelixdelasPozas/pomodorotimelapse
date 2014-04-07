/*
 * webmEBMLwriter.h
 *
 *  Created on: 23/03/2014
 *      Author: Felix de las Pozas Alvarez
 *
 *  Taken from libwebm library
 */

#ifndef WEBM_EBML_WRITER_H_
#define WEBM_EBML_WRITER_H_

// C++
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// libvpx
#include "vpx/vpx_integer.h"
#include "vpx/vpx_encoder.h"

struct EbmlGlobal
{
  FILE *stream;
  int64_t last_pts_ms;
  vpx_rational_t framerate;

  /* These pointers are to the start of an element */
  off_t position_reference;
  off_t seek_info_pos;
  off_t segment_info_pos;
  off_t track_pos;
  off_t cue_pos;
  off_t cluster_pos;

  /* This pointer is to a specific element to be serialized */
  off_t track_id_pos;

  /* These pointers are to the size field of the element */
  off_t startSegment;
  off_t startCluster;

  uint32_t cluster_timecode;
  int cluster_open;

  struct cue_entry *cue_list;
  unsigned int cues;
};

#define LITERALU64(hi, lo) ((((uint64_t)hi) << 32) | lo)

#define VP8_FOURCC (0x30385056)
#define VP9_FOURCC (0x30395056)
#define VP8_FOURCC_MASK (0x00385056)
#define VP9_FOURCC_MASK (0x00395056)

/* Stereo 3D packed frame format */
typedef enum stereo_format
{
  STEREO_FORMAT_MONO = 0,
  STEREO_FORMAT_LEFT_RIGHT = 1,
  STEREO_FORMAT_BOTTOM_TOP = 2,
  STEREO_FORMAT_TOP_BOTTOM = 3,
  STEREO_FORMAT_RIGHT_LEFT = 11
} stereo_format_t;

struct cue_entry
{
  unsigned int time;
  uint64_t loc;
};

unsigned int murmur(const void *key, int len, unsigned int seed);

typedef struct EbmlGlobal EbmlGlobal;

void Ebml_Serialize(EbmlGlobal *glob, const void *, int, unsigned long);
void Ebml_Write(EbmlGlobal *glob, const void *, unsigned long);

void Ebml_WriteLen(EbmlGlobal *glob, int64_t val);
void Ebml_WriteString(EbmlGlobal *glob, const char *str);
void Ebml_WriteUTF8(EbmlGlobal *glob, const wchar_t *wstr);
void Ebml_WriteID(EbmlGlobal *glob, unsigned long class_id);
void Ebml_SerializeUnsigned64(EbmlGlobal *glob, unsigned long class_id, uint64_t ui);
void Ebml_SerializeUnsigned32(EbmlGlobal *glob, unsigned int class_id, uint64_t ui);
void Ebml_SerializeUnsigned(EbmlGlobal *glob, unsigned long class_id, unsigned long ui);
void Ebml_SerializeBinary(EbmlGlobal *glob, unsigned long class_id, unsigned long ui);
void Ebml_SerializeFloat(EbmlGlobal *glob, unsigned long class_id, double d);

void Ebml_WriteSigned16(EbmlGlobal *glob, short val);
void Ebml_SerializeString(EbmlGlobal *glob, unsigned long class_id, const char *s);
void Ebml_SerializeUTF8(EbmlGlobal *glob, unsigned long class_id, wchar_t *s);
void Ebml_SerializeData(EbmlGlobal *glob, unsigned long class_id, unsigned char *data, unsigned long data_length);
void Ebml_WriteVoid(EbmlGlobal *glob, unsigned long vSize);

void Ebml_StartSubElement(EbmlGlobal *glob, off_t *ebmlLoc, unsigned int class_id);
void Ebml_EndSubElement(EbmlGlobal *glob, off_t *ebmlLoc);

void write_webm_seek_element(EbmlGlobal *ebml, unsigned int id, off_t pos);
void write_webm_file_header(EbmlGlobal *glob, const vpx_codec_enc_cfg_t *cfg, const struct vpx_rational *fps);
void write_webm_block(EbmlGlobal *glob, const vpx_codec_enc_cfg_t *cfg, const vpx_codec_cx_pkt_t *pkt);
void write_webm_file_footer(EbmlGlobal *glob, int hash);

#endif // WEBM_EBML_WRITER_H_