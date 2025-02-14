/*
    File: webmEBMLwriter.cpp
    Created on: 23/03/2014
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
#include <webmEBMLwriter.h>
#include <webmIDs.h>

// C++
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <limits.h>

// Qt
#include <QDebug>

//------------------------------------------------------------------
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
		// no break
		case 2:	h ^= data[1] << 8;
		// no break
		case 1:	h ^= data[0];
			h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

//------------------------------------------------------------------
void Ebml_Write(struct EbmlGlobal *global, const void *buffer_in, unsigned long len)
{
	(void) fwrite(buffer_in, 1, len, global->stream);
}

//------------------------------------------------------------------
#define WRITE_BUFFER(s) \
for (i = len - 1; i >= 0; i--) { \
  x = (char)(*(const s *)buffer_in >> (i * CHAR_BIT)); \
  Ebml_Write(global, &x, 1); \
}

void Ebml_Serialize(struct EbmlGlobal *global, const void *buffer_in, int buffer_size, unsigned long len)
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
		  Q_ASSERT(false);
			break;
	}
}
#undef WRITE_BUFFER

//------------------------------------------------------------------
void Ebml_WriteLen(EbmlGlobal *global, int64_t val)
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

	Ebml_Serialize(global, static_cast<void *>(&val), sizeof(val), size);
}

//------------------------------------------------------------------
void Ebml_WriteString(EbmlGlobal *global, const char *str)
{
	const size_t size_ = strlen(str);
	const uint64_t size = size_;
	Ebml_WriteLen(global, size);
	/* TODO: it's not clear from the spec whether the null terminator
	 * should be serialized too. For now we omit it.
	 */
	Ebml_Write(global, str, (unsigned long) size);
}

//------------------------------------------------------------------
void Ebml_WriteID(EbmlGlobal *global, unsigned long class_id)
{
	int len;

	if (class_id >= 0x01000000)
		len = 4;
	else
	{
		if (class_id >= 0x00010000)
			len = 3;
		else
		{
			if (class_id >= 0x00000100)
				len = 2;
			else
				len = 1;
		}
	}

	Ebml_Serialize(global, (void *) &class_id, sizeof(class_id), len);
}

//------------------------------------------------------------------
void Ebml_SerializeUnsigned64(EbmlGlobal *global, unsigned long class_id, uint64_t ui)
{
	const unsigned char sizeSerialized = 8 | 0x80;
	Ebml_WriteID(global, class_id);
	Ebml_Serialize(global, &sizeSerialized, sizeof(sizeSerialized), 1);
	Ebml_Serialize(global, &ui, sizeof(ui), 8);
}

//------------------------------------------------------------------
void Ebml_SerializeUnsigned32(struct EbmlGlobal *global, unsigned int class_id, uint64_t ui)
{
	const unsigned char sizeSerialized = 4 | 0x80;
	Ebml_WriteID(global, class_id);
	Ebml_Serialize(global, &sizeSerialized, sizeof(sizeSerialized), 1);
	Ebml_Serialize(global, &ui, sizeof(ui), 4);
}

//------------------------------------------------------------------
void Ebml_SerializeUnsigned(EbmlGlobal *global, unsigned long class_id, unsigned long ui)
{
	unsigned char size = 8; /* size in bytes to output */
	unsigned char sizeSerialized = 0;
	unsigned long minVal;

	Ebml_WriteID(global, class_id);
	minVal = 0x7fLU; /* mask to compare for byte size */

	for (size = 1; size < 4; size++)
	{
		if (ui < minVal)
			break;

		minVal <<= 7;
	}

	sizeSerialized = 0x80 | size;
	Ebml_Serialize(global, &sizeSerialized, sizeof(sizeSerialized), 1);
	Ebml_Serialize(global, &ui, sizeof(ui), size);
}

//------------------------------------------------------------------
void Ebml_SerializeBinary(EbmlGlobal *global, unsigned long class_id, unsigned long bin)
{
	int size;
	for (size = 4; size > 1; size--)
	{
		if (bin & (unsigned int) 0x000000ff << ((size - 1) * 8))
		  break;
	}
	Ebml_WriteID(global, class_id);
	Ebml_WriteLen(global, size);
	Ebml_WriteID(global, bin);
}

//------------------------------------------------------------------
void Ebml_SerializeFloat(EbmlGlobal *global, unsigned long class_id, double d)
{
	const unsigned char len = 0x88;
	Ebml_WriteID(global, class_id);
	Ebml_Serialize(global, &len, sizeof(len), 1);
	Ebml_Serialize(global, &d, sizeof(d), 8);
}

//------------------------------------------------------------------
void Ebml_SerializeString(EbmlGlobal *global, unsigned long class_id, const char *s)
{
	Ebml_WriteID(global, class_id);
	Ebml_WriteString(global, s);
}

//------------------------------------------------------------------
void Ebml_StartSubElement(EbmlGlobal *global, off_t *ebmlLoc, unsigned int class_id)
{
	const uint64_t kEbmlUnknownLength = LITERALU64(0x01FFFFFF, 0xFFFFFFFF);
	Ebml_WriteID(global, class_id);
	*ebmlLoc = ftello(global->stream);
	Ebml_Serialize(global, &kEbmlUnknownLength, sizeof(kEbmlUnknownLength), 8);
}

//------------------------------------------------------------------
void Ebml_EndSubElement(EbmlGlobal *global, off_t *ebmlLoc)
{
	off_t pos;
	uint64_t size;

	/* Save the current stream pointer. */
	pos = ftello(global->stream);

	/* Calculate the size of this element. */
	size = pos - *ebmlLoc - 8;
	size |= LITERALU64(0x01000000, 0x00000000);

	/* Seek back to the beginning of the element and write the new size. */
	fseeko(global->stream, *ebmlLoc, SEEK_SET);
	Ebml_Serialize(global, &size, sizeof(size), 8);

	/* Reset the stream pointer. */
	fseeko(global->stream, pos, SEEK_SET);
}

//------------------------------------------------------------------
void write_webm_seek_element(EbmlGlobal *global, unsigned int id, off_t pos)
{
	uint64_t offset = pos - global->position_reference;
	off_t start;
	Ebml_StartSubElement(global, &start, Seek);
	Ebml_SerializeBinary(global, SeekID, id);
	Ebml_SerializeUnsigned64(global, SeekPosition, offset);
	Ebml_EndSubElement(global, &start);
}

//------------------------------------------------------------------
void write_webm_seek_info(EbmlGlobal *global)
{
	off_t pos;
	off_t start;
	off_t startInfo;
	uint64_t frame_time;
	char version_string[64];

	/* Save the current stream pointer. */
	pos = ftello(global->stream);

	if (global->seek_info_pos)
		fseeko(global->stream, global->seek_info_pos, SEEK_SET);
	else
		global->seek_info_pos = pos;

	Ebml_StartSubElement(global, &start, SeekHead);
	write_webm_seek_element(global, Tracks, global->track_pos);
	write_webm_seek_element(global, Cues, global->cue_pos);
	write_webm_seek_element(global, Info, global->segment_info_pos);
	Ebml_EndSubElement(global, &start);

	/* Create and write the Segment Info. */
	strcpy(version_string, "DesktopCapture v1.0 - libVPX ");
	strncat(version_string, vpx_codec_version_str(), sizeof(version_string) - 1 - strlen(version_string));

	frame_time = (uint64_t) 1000 * global->framerate.den / global->framerate.num;

	global->segment_info_pos = ftello(global->stream);
	Ebml_StartSubElement(global, &startInfo, Info);
	Ebml_SerializeUnsigned(global, TimecodeScale, 1000000);
	Ebml_SerializeFloat(global, Segment_Duration, (double) (global->last_pts_ms + frame_time));
	Ebml_SerializeString(global, MuxingApp, version_string);
	Ebml_SerializeString(global, WritingApp, version_string);
	Ebml_EndSubElement(global, &startInfo);
}

//------------------------------------------------------------------
void write_webm_file_header(EbmlGlobal *global, const vpx_codec_enc_cfg_t *cfg, const struct vpx_rational *fps)
{
	off_t start;
	off_t trackStart;
	off_t videoStart;
	unsigned int trackNumber = 1;
	uint64_t trackID = 0;
	unsigned int pixelWidth = cfg->g_w;
	unsigned int pixelHeight = cfg->g_h;

	/* Write the EBML header. */
	Ebml_StartSubElement(global, &start, EBML);
	Ebml_SerializeUnsigned(global, EBMLVersion, 1);
	Ebml_SerializeUnsigned(global, EBMLReadVersion, 1);
	Ebml_SerializeUnsigned(global, EBMLMaxIDLength, 4);
	Ebml_SerializeUnsigned(global, EBMLMaxSizeLength, 8);
	Ebml_SerializeString(global, DocType, "webm");
	Ebml_SerializeUnsigned(global, DocTypeVersion, 2);
	Ebml_SerializeUnsigned(global, DocTypeReadVersion, 2);
	Ebml_EndSubElement(global, &start);

	/* Open and begin writing the segment element. */
	Ebml_StartSubElement(global, &global->startSegment, Segment);
	global->position_reference = ftello(global->stream);
	global->framerate = *fps;
	write_webm_seek_info(global);

	/* Open and write the Tracks element. */
	global->track_pos = ftello(global->stream);
	Ebml_StartSubElement(global, &trackStart, Tracks);

	/* Open and write the Track entry. */
	Ebml_StartSubElement(global, &start, TrackEntry);
	Ebml_SerializeUnsigned(global, TrackNumber, trackNumber);
	global->track_id_pos = ftello(global->stream);
	Ebml_SerializeUnsigned32(global, TrackUID, trackID);
	Ebml_SerializeUnsigned(global, TrackType, 1);
	Ebml_SerializeString(global, CodecID, "V_VP8");
	Ebml_StartSubElement(global, &videoStart, Video);
	Ebml_SerializeUnsigned(global, PixelWidth, pixelWidth);
	Ebml_SerializeUnsigned(global, PixelHeight, pixelHeight);
	Ebml_SerializeUnsigned(global, StereoMode, STEREO_FORMAT_MONO);
	Ebml_EndSubElement(global, &videoStart);

	/* Close Track entry. */
	Ebml_EndSubElement(global, &start);

	/* Close Tracks element. */
	Ebml_EndSubElement(global, &trackStart);

	/* Segment element remains open. */
}

//------------------------------------------------------------------
void write_webm_block(EbmlGlobal *global, const vpx_codec_enc_cfg_t *cfg, const vpx_codec_cx_pkt_t *pkt)
{
	unsigned int block_length;
	unsigned char track_number;
	uint16_t block_timecode = 0;
	unsigned char flags;
	int64_t pts_ms;
	int start_cluster = 0, is_keyframe;

	/* Calculate the PTS of this frame in milliseconds. */
	pts_ms = pkt->data.frame.pts * 1000 * static_cast<uint64_t>(cfg->g_timebase.num) / (uint64_t) cfg->g_timebase.den;

	if (pts_ms <= global->last_pts_ms)
		pts_ms = global->last_pts_ms + 1;

	global->last_pts_ms = pts_ms;

	/* Calculate the relative time of this block. */
	if (pts_ms - global->cluster_timecode > SHRT_MAX)
		start_cluster = 1;
	else
		block_timecode = static_cast<uint16_t>(pts_ms) - global->cluster_timecode;

	is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY);
	if (start_cluster || is_keyframe)
	{
		if (global->cluster_open)
			Ebml_EndSubElement(global, &global->startCluster);

		/* Open the new cluster. */
		block_timecode = 0;
		global->cluster_open = 1;
		global->cluster_timecode = static_cast<uint32_t>(pts_ms);
		global->cluster_pos = ftello(global->stream);
		Ebml_StartSubElement(global, &global->startCluster, Cluster);
		Ebml_SerializeUnsigned(global, Timecode, global->cluster_timecode);

		/* Save a cue point if this is a keyframe. */
		if (is_keyframe)
		{
			struct cue_entry *cue, *new_cue_list;

			new_cue_list = static_cast<struct cue_entry *>(realloc(global->cue_list, (global->cues + 1) * sizeof(struct cue_entry)));
			if (new_cue_list)
				global->cue_list = new_cue_list;
			else
			{
				qDebug("Failed to realloc cue list.");
				Q_ASSERT(false);
			}
			cue = &global->cue_list[global->cues];
			cue->time = global->cluster_timecode;
			cue->loc = global->cluster_pos;
			global->cues++;
		}
	}

	/* Write the Simple Block. */
	Ebml_WriteID(global, SimpleBlock);

	block_length = (unsigned int) pkt->data.frame.sz + 4;
	block_length |= 0x10000000;
	Ebml_Serialize(global, &block_length, sizeof(block_length), 4);

	track_number = 1;
	track_number |= 0x80;
	Ebml_Write(global, &track_number, 1);

	Ebml_Serialize(global, &block_timecode, sizeof(block_timecode), 2);

	flags = 0;
	if (is_keyframe)
		flags |= 0x80;

	if (pkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
		flags |= 0x08;

	Ebml_Write(global, &flags, 1);

	Ebml_Write(global, pkt->data.frame.buf, (unsigned int) pkt->data.frame.sz);
}

//------------------------------------------------------------------
void write_webm_file_footer(EbmlGlobal *global, int hash)
{
	off_t start_cues;
	off_t start_cue_point;
	off_t start_cue_tracks;
	unsigned int i;

	if (global->cluster_open)
		Ebml_EndSubElement(global, &global->startCluster);

	global->cue_pos = ftello(global->stream);
	Ebml_StartSubElement(global, &start_cues, Cues);

	for (i = 0; i < global->cues; i++)
	{
		struct cue_entry *cue = &global->cue_list[i];
		Ebml_StartSubElement(global, &start_cue_point, CuePoint);
		Ebml_SerializeUnsigned(global, CueTime, cue->time);

		Ebml_StartSubElement(global, &start_cue_tracks, CueTrackPositions);
		Ebml_SerializeUnsigned(global, CueTrack, 1);
		Ebml_SerializeUnsigned64(global, CueClusterPosition, cue->loc - global->position_reference);
		Ebml_EndSubElement(global, &start_cue_tracks);

		Ebml_EndSubElement(global, &start_cue_point);
	}

	Ebml_EndSubElement(global, &start_cues);

	/* Close the Segment. */
	Ebml_EndSubElement(global, &global->startSegment);

	/* Patch up the seek info block. */
	write_webm_seek_info(global);

	/* Patch up the track id. */
	fseeko(global->stream, global->track_id_pos, SEEK_SET);
	Ebml_SerializeUnsigned32(global, TrackUID, hash);

	fseeko(global->stream, 0, SEEK_END);
}
