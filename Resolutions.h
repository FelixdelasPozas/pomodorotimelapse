/*
 * Resolutions.h
 *
 *  Created on: 17/01/2014
 *      Author: Felix de las Pozas Alvarez
 */

#ifndef COMMON_RESOLUTIONS_LIST_H_
#define COMMON_RESOLUTIONS_LIST_H_

#include <QString>
#include <QList>

struct Resolution
{
	QString name;
	double width;
	double height;

	Resolution(QString rName, double rWidth, double rHeight): name{rName}, width{rWidth}, height{rHeight} {};
	Resolution(): name{QString()}, width{0}, height{0} {};

	bool operator==(const Resolution &lhs) const
	{
		return ((lhs.name == name) && (lhs.width == width) && (lhs.height == height));
	}
};


using ResolutionList = QList<Resolution>;

static const ResolutionList CommonResolutions = { { QString("SQCIF"), 128, 96 },
																									{ QString("QQVGA"), 160, 120 },
																									{ QString("QCIF"), 176, 144 },
																									{ QString("HQVGA"), 240, 160 },
																									{ QString("SCIF"), 256, 192 },
																									{ QString("QVGA"), 320, 240 },
																									{ QString("SIF(525)"), 352, 240 },
																									{ QString("WQVGA"), 360, 240 },
																									{ QString("WQVGA"), 384, 240 },
																									{ QString("WQVGA"), 400, 240 },
																									{ QString("FWQVGA"), 432, 240 },
																									{ QString("CIF/SIF(625)"), 352, 288 },
																									{ QString("HVGA"), 480, 320 },
																									{ QString("Unknown"), 480, 360 },
																									{ QString("nHD"), 640, 360 },
																									{ QString("DCIF"), 528, 384 },
																									{ QString("VGA"), 640, 480 },
																									{ QString("4SIF(525)"), 704, 480 },
																									{ QString("WVGA"), 720, 480 },
																									{ QString("WVGA"), 800, 480 },
																									{ QString("FWVGA"), 854, 480 },
																									{ QString("qHD"), 960, 540 },
																									{ QString("4CIF/4SIF(625)"), 704, 576 },
																									{ QString("WSVGA"), 1024, 576 },
																									{ QString("SVGA"), 800, 600 },
																									{ QString("WSVGA"), 1024, 600 },
																									{	QString("DVGA"), 960, 640 },
																									{ QString("Unknown"), 1024, 640 },
																									{ QString("Unknown"), 1136, 640 },
																									{ QString("Unknown"), 1152, 720 },
																									{ QString("HD/WXGA"),	1280, 720 },
																									{ QString("XGA"), 1024, 768 },
																									{ QString("WXGA"), 1152, 768 },
																									{ QString("WXGA"), 1280, 768 },
																									{ QString("WXGA"), 1366, 768 },
																									{ QString("WXGA"), 1280, 800 },
																									{ QString("WXGA"), 1152, 864 },
																									{ QString("WXGA"), 1280, 864 },
																									{ QString("WSXGA"), 1440, 900 },
																									{ QString("HD+"), 1600, 900 },
																									{	QString("SXGA-"), 1280, 960 },
																									{ QString("WSXGA"), 1440, 960 },
																									{ QString("SXGA"), 1280, 1024 },
																									{ QString("SXGA+"), 1400, 1050 },
																									{ QString("WSXGA+"), 1680, 1050 },
																									{ QString("FHD"), 1920, 1080 },
																									{ QString("16CIF"), 1408, 1152 },
																									{ QString("QWXGA"), 2048, 1152 },
																									{ QString("UXGA"), 1600, 1200 },
																									{ QString("WUXGA"), 1920, 1200 },
																									{	QString("(W)QHD"), 2560, 1440 },
																									{ QString("QXGA"), 2048, 1536 },
																									{ QString("WQXGA"), 2560, 1600 },
																									{ QString("Unknown"), 2880, 1620 },
																									{ QString("Unknown"), 2880, 1800 },
																									{ QString("WQXGA+"), 3200, 1800 },
																									{ QString("QSXGA"), 2560, 2048 },
																									{ QString("WQSXGA"), 3200, 2048 },
																									{ QString("UHD"), 3840,	2160 },
																									{ QString("QUXGA"), 3200, 2400 },
																									{ QString("WQUXGA"), 3840, 2400 },
																									{ QString("4K"), 4096, 2560 },
																									{ QString("UHD+"), 5120, 2880 },
																									{ QString("HXGA"), 4096, 3072 },
																									{ QString("WHXGA"), 5120, 3200 },
																									{ QString("Unknown"), 5760, 3240 },
																									{ QString("HSXGA"), 5120, 4096 },
																									{ QString("WHSXGA"), 6400, 4096 },
																									{ QString("FUHD(8K)"), 7680, 4320 },
																									{ QString("HUXGA"), 6400, 4800 },
																									{ QString("WHUXGA"), 7680, 4800 },
																									{ QString("QUHD(16K)"), 15360, 8640 } };

static QString getResolutionAsString(Resolution res)
{
	return QString("%1x%2 - ").arg(res.width).arg(res.height) + res.name;
}

static Resolution getResolution(int width, int height)
{
	Resolution match;

	for (Resolution resolution: CommonResolutions)
		if (resolution.width == width && resolution.height == height)
		{
			match = resolution;
			break;
		}

	return match;
}

#endif // COMMON_RESOLUTIONS_LIST_H_ */

























