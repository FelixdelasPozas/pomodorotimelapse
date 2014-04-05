/*
 * webmEBMLwriter.c
 *
 *  Created on: 23/03/2014
 *      Author: Felix de las Pozas Alvarez
 *
 *  Taken from libwebm library
 */

#include "webmEBMLwriter.h"
#include "webmIDs.h"

// C++
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <limits.h>

#include <QDebug>

/* Murmur hash derived from public domain reference implementation at
 *   http:// sites.google.com/site/murmurhash/
 */
unsigned int murmur(const void *key, int len, unsigned int seed)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	unsigned int h = seed ^ len;

	const unsigned char *data = (const unsigned char *) key;

	while (len >= 4)
	{
		unsigned int k;

		k = (unsigned int) data[0];
		k |= (unsigned int) data[1] << 8;
		k |= (unsigned int) data[2] << 16;
		k |= (unsigned int) data[3] << 24;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	switch (len)
	{
		case 3:	h ^= data[2] << 16;
		case 2:	h ^= data[1] << 8;
		case 1:	h ^= data[0];
			h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

//------------------------------------------------------------------
void Ebml_Write(struct EbmlGlobal *glob, const void *buffer_in, unsigned long len)
{
	(void) fwrite(buffer_in, 1, len, glob->stream);
}

//------------------------------------------------------------------
#define WRITE_BUFFER(s) \
for (i = len - 1; i >= 0; i--) { \
  x = (char)(*(const s *)buffer_in >> (i * CHAR_BIT)); \
  Ebml_Write(glob, &x, 1); \
}

void Ebml_Serialize(struct EbmlGlobal *glob, const void *buffer_in, int buffer_size, unsigned long len)
{
	char x;
	int i;

	/* buffer_size:
	 * 1 - int8_t;
	 * 2 - int16_t;
	 * 3 - int32_t;
	 * 4 - int64_t;
	 */
	switch (buffer_size)
	{
		case 1:
			WRITE_BUFFER(int8_t)
			break;
		case 2:
			WRITE_BUFFER(int16_t)
			break;
		case 4:
			WRITE_BUFFER(int32_t)
			break;
		case 8:
			WRITE_BUFFER(int64_t)
			break;
		default:
			break;
	}
}
#undef WRITE_BUFFER

//------------------------------------------------------------------
void Ebml_WriteLen(EbmlGlobal *glob, int64_t val)
{
	/* TODO check and make sure we are not > than 0x0100000000000000LLU */
	unsigned char size = 8; /* size in bytes to output */

	/* mask to compare for byte size */
	int64_t minVal = 0xff;

	for (size = 1; size < 8; size++)
	{
		if (val < minVal)
			break;

		minVal = (minVal << 7);
	}

	val |= ((static_cast<uint64_t>(0x80)) << ((size - 1) * 7));

	Ebml_Serialize(glob, static_cast<void *>(&val), sizeof(val), size);
}

//------------------------------------------------------------------
void Ebml_WriteString(EbmlGlobal *glob, const char *str)
{
	const size_t size_ = strlen(str);
	const uint64_t size = size_;
	Ebml_WriteLen(glob, size);
	/* TODO: it's not clear from the spec whether the nul terminator
	 * should be serialized too.  For now we omit the null terminator.
	 */
	Ebml_Write(glob, str, (unsigned long) size);
}

//------------------------------------------------------------------
void Ebml_WriteUTF8(EbmlGlobal *glob, const wchar_t *wstr)
{
	const size_t strlen = wcslen(wstr);

	/* TODO: it's not clear from the spec whether the nul terminator
	 * should be serialized too.  For now we include it.
	 */
	const uint64_t size = strlen;

	Ebml_WriteLen(glob, size);
	Ebml_Write(glob, wstr, (unsigned long) size);
}

//------------------------------------------------------------------
void Ebml_WriteID(EbmlGlobal *glob, unsigned long class_id)
{
	int len;

	if (class_id >= 0x01000000)
		len = 4;
	else
		if (class_id >= 0x00010000)
			len = 3;
		else
			if (class_id >= 0x00000100)
				len = 2;
			else
				len = 1;

	Ebml_Serialize(glob, (void *) &class_id, sizeof(class_id), len);
}

//------------------------------------------------------------------
void Ebml_SerializeUnsigned64(EbmlGlobal *glob, unsigned long class_id, uint64_t ui)
{
	unsigned char sizeSerialized = 8 | 0x80;
	Ebml_WriteID(glob, class_id);
	Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
	Ebml_Serialize(glob, &ui, sizeof(ui), 8);
}

//------------------------------------------------------------------
void Ebml_SerializeUnsigned32(struct EbmlGlobal *glob, unsigned int class_id, uint64_t ui)
{
	const unsigned char sizeSerialized = 4 | 0x80;
	Ebml_WriteID(glob, class_id);
	Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
	Ebml_Serialize(glob, &ui, sizeof(ui), 4);
}

//------------------------------------------------------------------
void Ebml_SerializeUnsigned(EbmlGlobal *glob, unsigned long class_id, unsigned long ui)
{
	unsigned char size = 8; /* size in bytes to output */
	unsigned char sizeSerialized = 0;
	unsigned long minVal;

	Ebml_WriteID(glob, class_id);
	minVal = 0x7fLU; /* mask to compare for byte size */

	for (size = 1; size < 4; size++)
	{
		if (ui < minVal)
		{
			break;
		}

		minVal <<= 7;
	}

	sizeSerialized = 0x80 | size;
	Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
	Ebml_Serialize(glob, &ui, sizeof(ui), size);
}

//------------------------------------------------------------------
void Ebml_SerializeBinary(EbmlGlobal *glob, unsigned long class_id, unsigned long bin)
{
	int size;
	for (size = 4; size > 1; size--)
	{
		if (bin & (unsigned int) 0x000000ff << ((size - 1) * 8))
			break;
	}
	Ebml_WriteID(glob, class_id);
	Ebml_WriteLen(glob, size);
	Ebml_WriteID(glob, bin);
}

//------------------------------------------------------------------
void Ebml_SerializeFloat(EbmlGlobal *glob, unsigned long class_id, double d)
{
	unsigned char len = 0x88;

	Ebml_WriteID(glob, class_id);
	Ebml_Serialize(glob, &len, sizeof(len), 1);
	Ebml_Serialize(glob, &d, sizeof(d), 8);
}

//------------------------------------------------------------------
void Ebml_WriteSigned16(EbmlGlobal *glob, short val)
{
	signed long out = ((val & 0x003FFFFF) | 0x00200000) << 8;
	Ebml_Serialize(glob, &out, sizeof(out), 3);
}

//------------------------------------------------------------------
void Ebml_SerializeString(EbmlGlobal *glob, unsigned long class_id, const char *s)
{
	Ebml_WriteID(glob, class_id);
	Ebml_WriteString(glob, s);
}

//------------------------------------------------------------------
void Ebml_SerializeUTF8(EbmlGlobal *glob, unsigned long class_id, wchar_t *s)
{
	Ebml_WriteID(glob, class_id);
	Ebml_WriteUTF8(glob, s);
}

//------------------------------------------------------------------
void Ebml_SerializeData(EbmlGlobal *glob, unsigned long class_id, unsigned char *data, unsigned long data_length)
{
	Ebml_WriteID(glob, class_id);
	Ebml_WriteLen(glob, data_length);
	Ebml_Write(glob, data, data_length);
}

//------------------------------------------------------------------
void Ebml_WriteVoid(EbmlGlobal *glob, unsigned long vSize)
{
	unsigned char tmp = 0;
	unsigned long i = 0;

	Ebml_WriteID(glob, Void);
	Ebml_WriteLen(glob, vSize);

	for (i = 0; i < vSize; i++)
	{
		Ebml_Write(glob, &tmp, 1);
	}
}

//------------------------------------------------------------------
void Ebml_StartSubElement(EbmlGlobal *glob, off_t *ebmlLoc, unsigned int class_id)
{
	const uint64_t kEbmlUnknownLength = LITERALU64(0x01FFFFFF, 0xFFFFFFFF);
	Ebml_WriteID(glob, class_id);
	*ebmlLoc = ftello(glob->stream);
	Ebml_Serialize(glob, &kEbmlUnknownLength, sizeof(kEbmlUnknownLength), 8);
}

//------------------------------------------------------------------
void Ebml_EndSubElement(EbmlGlobal *glob, off_t *ebmlLoc)
{
	off_t pos;
	uint64_t size;

	/* Save the current stream pointer. */
	pos = ftello(glob->stream);

	/* Calculate the size of this element. */
	size = pos - *ebmlLoc - 8;
	size |= LITERALU64(0x01000000, 0x00000000);

	/* Seek back to the beginning of the element and write the new size. */
	fseeko(glob->stream, *ebmlLoc, SEEK_SET);
	Ebml_Serialize(glob, &size, sizeof(size), 8);

	/* Reset the stream pointer. */
	fseeko(glob->stream, pos, SEEK_SET);
}

//------------------------------------------------------------------
void write_webm_seek_element(EbmlGlobal *ebml, unsigned int id, off_t pos)
{
	uint64_t offset = pos - ebml->position_reference;
	off_t start;
	Ebml_StartSubElement(ebml, &start, Seek);
	Ebml_SerializeBinary(ebml, SeekID, id);
	Ebml_SerializeUnsigned64(ebml, SeekPosition, offset);
	Ebml_EndSubElement(ebml, &start);
}

//------------------------------------------------------------------
void write_webm_seek_info(EbmlGlobal *ebml)
{
	off_t pos;
	off_t start;
	off_t startInfo;
	uint64_t frame_time;
	char version_string[64];

	/* Save the current stream pointer. */
	pos = ftello(ebml->stream);

	if (ebml->seek_info_pos)
		fseeko(ebml->stream, ebml->seek_info_pos, SEEK_SET);
	else
		ebml->seek_info_pos = pos;

	Ebml_StartSubElement(ebml, &start, SeekHead);
	write_webm_seek_element(ebml, Tracks, ebml->track_pos);
	write_webm_seek_element(ebml, Cues, ebml->cue_pos);
	write_webm_seek_element(ebml, Info, ebml->segment_info_pos);
	Ebml_EndSubElement(ebml, &start);

	/* Create and write the Segment Info. */
	strcpy(version_string, "DesktopCapture v1.0 - libVPX ");
	strncat(version_string, vpx_codec_version_str(), sizeof(version_string) - 1 - strlen(version_string));

	frame_time = (uint64_t) 1000 * ebml->framerate.den / ebml->framerate.num;

	ebml->segment_info_pos = ftello(ebml->stream);
	Ebml_StartSubElement(ebml, &startInfo, Info);
	Ebml_SerializeUnsigned(ebml, TimecodeScale, 1000000);
	Ebml_SerializeFloat(ebml, Segment_Duration, (double) (ebml->last_pts_ms + frame_time));
	Ebml_SerializeString(ebml, MuxingApp, version_string);
	Ebml_SerializeString(ebml, WritingApp, version_string);
	Ebml_EndSubElement(ebml, &startInfo);
}

//------------------------------------------------------------------
void write_webm_file_header(EbmlGlobal *glob, const vpx_codec_enc_cfg_t *cfg, const struct vpx_rational *fps)
{
	off_t start;
	off_t trackStart;
	off_t videoStart;
	unsigned int trackNumber = 1;
	uint64_t trackID = 0;
	unsigned int pixelWidth = cfg->g_w;
	unsigned int pixelHeight = cfg->g_h;

	/* Write the EBML header. */
	Ebml_StartSubElement(glob, &start, EBML);
	Ebml_SerializeUnsigned(glob, EBMLVersion, 1);
	Ebml_SerializeUnsigned(glob, EBMLReadVersion, 1);
	Ebml_SerializeUnsigned(glob, EBMLMaxIDLength, 4);
	Ebml_SerializeUnsigned(glob, EBMLMaxSizeLength, 8);
	Ebml_SerializeString(glob, DocType, "webm");
	Ebml_SerializeUnsigned(glob, DocTypeVersion, 2);
	Ebml_SerializeUnsigned(glob, DocTypeReadVersion, 2);
	Ebml_EndSubElement(glob, &start);

	/* Open and begin writing the segment element. */
	Ebml_StartSubElement(glob, &glob->startSegment, Segment);
	glob->position_reference = ftello(glob->stream);
	glob->framerate = *fps;
	write_webm_seek_info(glob);

	/* Open and write the Tracks element. */
	glob->track_pos = ftello(glob->stream);
	Ebml_StartSubElement(glob, &trackStart, Tracks);

	/* Open and write the Track entry. */
	Ebml_StartSubElement(glob, &start, TrackEntry);
	Ebml_SerializeUnsigned(glob, TrackNumber, trackNumber);
	glob->track_id_pos = ftello(glob->stream);
	Ebml_SerializeUnsigned32(glob, TrackUID, trackID);
	Ebml_SerializeUnsigned(glob, TrackType, 1);
	Ebml_SerializeString(glob, CodecID, "V_VP8");
	Ebml_StartSubElement(glob, &videoStart, Video);
	Ebml_SerializeUnsigned(glob, PixelWidth, pixelWidth);
	Ebml_SerializeUnsigned(glob, PixelHeight, pixelHeight);
	Ebml_SerializeUnsigned(glob, StereoMode, STEREO_FORMAT_MONO);
	Ebml_EndSubElement(glob, &videoStart);

	/* Close Track entry. */
	Ebml_EndSubElement(glob, &start);

	/* Close Tracks element. */
	Ebml_EndSubElement(glob, &trackStart);

	/* Segment element remains open. */
}

//------------------------------------------------------------------
void write_webm_block(EbmlGlobal *glob, const vpx_codec_enc_cfg_t *cfg, const vpx_codec_cx_pkt_t *pkt)
{
	unsigned int block_length;
	unsigned char track_number;
	uint16_t block_timecode = 0;
	unsigned char flags;
	int64_t pts_ms;
	int start_cluster = 0, is_keyframe;

	/* Calculate the PTS of this frame in milliseconds. */
	pts_ms = pkt->data.frame.pts * 1000 * static_cast<uint64_t>(cfg->g_timebase.num) / (uint64_t) cfg->g_timebase.den;

	if (pts_ms <= glob->last_pts_ms)
		pts_ms = glob->last_pts_ms + 1;

	glob->last_pts_ms = pts_ms;

	/* Calculate the relative time of this block. */
	if (pts_ms - glob->cluster_timecode > SHRT_MAX)
		start_cluster = 1;
	else
		block_timecode = static_cast<uint16_t>(pts_ms) - glob->cluster_timecode;

	is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY);
	if (start_cluster || is_keyframe)
	{
		if (glob->cluster_open)
			Ebml_EndSubElement(glob, &glob->startCluster);
		/* Open the new cluster. */
		block_timecode = 0;
		glob->cluster_open = 1;
		glob->cluster_timecode = static_cast<uint32_t>(pts_ms);
		glob->cluster_pos = ftello(glob->stream);
		Ebml_StartSubElement(glob, &glob->startCluster, Cluster);
		Ebml_SerializeUnsigned(glob, Timecode, glob->cluster_timecode);

		/* Save a cue point if this is a keyframe. */
		if (is_keyframe)
		{
			struct cue_entry *cue, *new_cue_list;

			new_cue_list = static_cast<struct cue_entry *>(realloc(glob->cue_list, (glob->cues + 1) * sizeof(struct cue_entry)));
			if (new_cue_list)
				glob->cue_list = new_cue_list;
			else
			{
				qDebug("Failed to realloc cue list.");
				Q_ASSERT(false);
			}
			cue = &glob->cue_list[glob->cues];
			cue->time = glob->cluster_timecode;
			cue->loc = glob->cluster_pos;
			glob->cues++;
		}
	}

	/* Write the Simple Block. */
	Ebml_WriteID(glob, SimpleBlock);

	block_length = (unsigned int) pkt->data.frame.sz + 4;
	block_length |= 0x10000000;
	Ebml_Serialize(glob, &block_length, sizeof(block_length), 4);

	track_number = 1;
	track_number |= 0x80;
	Ebml_Write(glob, &track_number, 1);

	Ebml_Serialize(glob, &block_timecode, sizeof(block_timecode), 2);

	flags = 0;
	if (is_keyframe)
		flags |= 0x80;
	if (pkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
		flags |= 0x08;
	Ebml_Write(glob, &flags, 1);

	Ebml_Write(glob, pkt->data.frame.buf, (unsigned int) pkt->data.frame.sz);
}

//------------------------------------------------------------------
void write_webm_file_footer(EbmlGlobal *glob, int hash)
{
	off_t start_cues;
	off_t start_cue_point;
	off_t start_cue_tracks;
	unsigned int i;

	if (glob->cluster_open)
		Ebml_EndSubElement(glob, &glob->startCluster);

	glob->cue_pos = ftello(glob->stream);
	Ebml_StartSubElement(glob, &start_cues, Cues);

	for (i = 0; i < glob->cues; i++)
	{
		struct cue_entry *cue = &glob->cue_list[i];
		Ebml_StartSubElement(glob, &start_cue_point, CuePoint);
		Ebml_SerializeUnsigned(glob, CueTime, cue->time);

		Ebml_StartSubElement(glob, &start_cue_tracks, CueTrackPositions);
		Ebml_SerializeUnsigned(glob, CueTrack, 1);
		Ebml_SerializeUnsigned64(glob, CueClusterPosition, cue->loc - glob->position_reference);
		Ebml_EndSubElement(glob, &start_cue_tracks);

		Ebml_EndSubElement(glob, &start_cue_point);
	}

	Ebml_EndSubElement(glob, &start_cues);

	/* Close the Segment. */
	Ebml_EndSubElement(glob, &glob->startSegment);

	/* Patch up the seek info block. */
	write_webm_seek_info(glob);

	/* Patch up the track id. */
	fseeko(glob->stream, glob->track_id_pos, SEEK_SET);
	Ebml_SerializeUnsigned32(glob, TrackUID, hash);

	fseeko(glob->stream, 0, SEEK_END);
}
