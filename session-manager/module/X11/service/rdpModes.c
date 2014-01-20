/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013-2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "rdp.h"
#include "rdpRandr.h"
#include "rdpScreen.h"

#include "rdpModes.h"

#include <winpr/crt.h>
#include <winpr/path.h>
#include <winpr/file.h>
#include <winpr/stream.h>

#define DEFINE_SCREEN_SIZE(_width, _height) ((_width << 16) | _height)

#define SCREEN_SIZE_WIDTH(_size) ((_size >> 16) & 0xFFFF)
#define SCREEN_SIZE_HEIGHT(_size) ((_size) & 0xFFFF)

UINT32 g_StandardSizes[24] =
{
	DEFINE_SCREEN_SIZE(8192, 4608),
	DEFINE_SCREEN_SIZE(4096, 2560),
	DEFINE_SCREEN_SIZE(4096, 2160),
	DEFINE_SCREEN_SIZE(3840, 2160),
	DEFINE_SCREEN_SIZE(2560, 1600), /* 16:10 */
	DEFINE_SCREEN_SIZE(2560, 1440),
	DEFINE_SCREEN_SIZE(2048, 1152),
	DEFINE_SCREEN_SIZE(2048, 1080),
	DEFINE_SCREEN_SIZE(1920, 1200), /* 16:10 */
	DEFINE_SCREEN_SIZE(1920, 1080),
	DEFINE_SCREEN_SIZE(1680, 1050), /* 16:10 */
	DEFINE_SCREEN_SIZE(1600, 1200),
	DEFINE_SCREEN_SIZE(1600, 900),
	DEFINE_SCREEN_SIZE(1440, 900), /* 16:10 */
	DEFINE_SCREEN_SIZE(1400, 1050),
	DEFINE_SCREEN_SIZE(1366, 768),
	DEFINE_SCREEN_SIZE(1280, 1024),
	DEFINE_SCREEN_SIZE(1280, 960),
	DEFINE_SCREEN_SIZE(1280, 800), /* 16:10 */
	DEFINE_SCREEN_SIZE(1280, 720),
	DEFINE_SCREEN_SIZE(1152, 864),
	DEFINE_SCREEN_SIZE(1024, 768),
	DEFINE_SCREEN_SIZE(800, 600),
	DEFINE_SCREEN_SIZE(640, 480)
};

EDID* rdpConstructScreenEdid(ScreenPtr pScreen)
{
	EDID* edid;

	edid = (EDID*) malloc(sizeof(EDID));

	if (!edid)
		return NULL;

	/**
	 * 00 ff ff ff ff ff ff 00 Header
	 * 1e 6d ManufacturerId
	 * 8d 57 ManufaturerProductCode
	 * 36 21 01 00 ManufacturerSerialNumber
	 * 0a WeekOfManufacture
	 * 14 YearOfManufacture
	 * 01 EdidVersion
	 * 03 EdidRevision
	 * e0 30 1b 78 DisplayParameters
	 * ea 33 37 a5 55 4d 9d 25 11 50 ChromacityCoordinates
	 * 52 a5 4b 00 BitmapTiming
	 * b3 00 81 80 81 8f 71 4f 01 01 01 01 01 01 01 01 StandardTiming
	 * 02 3a 80 18 71 38 2d 40 58 2c 45 00 dd 0c 11 00 00 1a Descriptor1
	 * 00 00 00 fd 00 38 4b 1e 53 0f 00 0a 20 20 20 20 20 20 Descriptor2
	 * 00 00 00 fc 00 45 32 32 35 30 0a 20 20 20 20 20 20 20 Descriptor3
	 * 00 00 00 ff 00 30 31 30 4e 44 52 46 32 36 30 33 38 0a Descriptor4
	 * 00 NumberOfExtensions
	 * a2 Checksum
	 */

	/**
	 * 00 ff ff ff ff ff ff 00 Header
	 * 5a 63 ManufacturerId
	 * 2d 45 ManufaturerProductCode
	 * 01 01 01 01 ManufacturerSerialNumber
	 * 05 WeekOfManufacture
	 * 17 YearOfManufacture
	 * 01 EdidVersion
	 * 03 EdidRevision
	 * 80 34 1d 78 DisplayParameters
	 * 2e a7 25 a4 57 51 a0 26 10 50 ChromacityCoordinates
	 * 54 bf ef 80 BitmapTiming
	 * b3 00 a9 40 a9 c0 95 00 90 40 81 80 81 40 81 00 StandardTiming
	 * 02 3a 80 18 71 38 2d 40 58 2c 45 00 09 25 21 00 00 1e Descriptor1
	 * 00 00 00 ff 00 54 4a 47 31 33 30 35 30 30 34 34 34 0a Descriptor2
	 * 00 00 00 fd 00 32 4b 1e 52 11 00 0a 20 20 20 20 20 20 Descriptor3
	 * 00 00 00 fc 00 54 44 32 34 32 30 20 53 45 52 49 45 53 Descriptor4
	 * 00 NumberOfExtensions
	 * f0 Checksum
	 */

	ZeroMemory(edid, sizeof(EDID));

	edid->Header[0] = 0x00;
	edid->Header[1] = 0xFF;
	edid->Header[2] = 0xFF;
	edid->Header[3] = 0xFF;
	edid->Header[4] = 0xFF;
	edid->Header[5] = 0xFF;
	edid->Header[6] = 0xFF;
	edid->Header[7] = 0x00;

	edid->ManufacturerId = 0x635A; /* 5a 63 */
	edid->ManufacturerProductCode = 0x452D; /* 2d 45 */
	edid->ManufacturerSerialNumber = 0x01010101; /* 01 01 01 01 */

	edid->WeekOfManufacture = 10; /* 0x0A */
	edid->YearOfManufacture = 20; /* 0x14 */

	edid->EdidVersion = 1;
	edid->EdidRevision = 3;

	/* 80 34 1d 78 */
	edid->DisplayParameters[0] = 0x80;
	edid->DisplayParameters[1] = 0x34;
	edid->DisplayParameters[2] = 0x1D;
	edid->DisplayParameters[3] = 0x78;

	/* 2e a7 25 a4 57 51 a0 26 10 50 */
	edid->ChromacityCoordinates[0] = 0x2E;
	edid->ChromacityCoordinates[1] = 0xA7;
	edid->ChromacityCoordinates[2] = 0x25;
	edid->ChromacityCoordinates[3] = 0xA4;
	edid->ChromacityCoordinates[4] = 0x57;
	edid->ChromacityCoordinates[5] = 0x51;
	edid->ChromacityCoordinates[6] = 0xA0;
	edid->ChromacityCoordinates[7] = 0x26;
	edid->ChromacityCoordinates[8] = 0x10;
	edid->ChromacityCoordinates[9] = 0x50;

	/* 54 bf ef 80 */
	edid->BitmapTiming[0] = 0x54;
	edid->BitmapTiming[1] = 0xBF;
	edid->BitmapTiming[2] = 0xEF;
	edid->BitmapTiming[3] = 0x80;

	/* b3 00 a9 40 a9 c0 95 00 90 40 81 80 81 40 81 00 */
	edid->StandardTiming[0] = 0xB3;
	edid->StandardTiming[1] = 0x00;
	edid->StandardTiming[2] = 0xA9;
	edid->StandardTiming[3] = 0x40;
	edid->StandardTiming[4] = 0xA9;
	edid->StandardTiming[5] = 0xC0;
	edid->StandardTiming[6] = 0x95;
	edid->StandardTiming[7] = 0x00;
	edid->StandardTiming[8] = 0x90;
	edid->StandardTiming[9] = 0x40;
	edid->StandardTiming[10] = 0x81;
	edid->StandardTiming[11] = 0x80;
	edid->StandardTiming[12] = 0x81;
	edid->StandardTiming[13] = 0x40;
	edid->StandardTiming[14] = 0x81;
	edid->StandardTiming[15] = 0x00;

	/* 02 3a 80 18 71 38 2d 40 58 2c 45 00 dd 0c 11 00 00 1a */
	edid->Descriptor1[0] = 0x02;
	edid->Descriptor1[1] = 0x3A;
	edid->Descriptor1[2] = 0x80;
	edid->Descriptor1[3] = 0x18;
	edid->Descriptor1[4] = 0x71;
	edid->Descriptor1[5] = 0x38;
	edid->Descriptor1[6] = 0x2D;
	edid->Descriptor1[7] = 0x40;
	edid->Descriptor1[8] = 0x58;
	edid->Descriptor1[9] = 0x2C;
	edid->Descriptor1[10] = 0x45;
	edid->Descriptor1[11] = 0x00;
	edid->Descriptor1[12] = 0xDD;
	edid->Descriptor1[13] = 0x0C;
	edid->Descriptor1[14] = 0x11;
	edid->Descriptor1[15] = 0x00;
	edid->Descriptor1[16] = 0x00;
	edid->Descriptor1[17] = 0x1A;

	/* 00 00 00 fd 00 38 4b 1e 53 0f 00 0a 20 20 20 20 20 20 */
	edid->Descriptor2[0] = 0x00;
	edid->Descriptor2[1] = 0x00;
	edid->Descriptor2[2] = 0x00;
	edid->Descriptor2[3] = 0xFD;
	edid->Descriptor2[4] = 0x00;
	edid->Descriptor2[5] = 0x38;
	edid->Descriptor2[6] = 0x4B;
	edid->Descriptor2[7] = 0x1E;
	edid->Descriptor2[8] = 0x53;
	edid->Descriptor2[9] = 0x0F;
	edid->Descriptor2[10] = 0x00;
	edid->Descriptor2[11] = 0x0A;
	edid->Descriptor2[12] = 0x20;
	edid->Descriptor2[13] = 0x20;
	edid->Descriptor2[14] = 0x20;
	edid->Descriptor2[15] = 0x20;
	edid->Descriptor2[16] = 0x20;
	edid->Descriptor2[17] = 0x20;

	/* 00 00 00 fc 00 45 32 32 35 30 0a 20 20 20 20 20 20 20 */
	edid->Descriptor3[0] = 0x00;
	edid->Descriptor3[1] = 0x00;
	edid->Descriptor3[2] = 0x00;
	edid->Descriptor3[3] = 0xFC;
	edid->Descriptor3[4] = 0x00;
	edid->Descriptor3[5] = 0x45;
	edid->Descriptor3[6] = 0x32;
	edid->Descriptor3[7] = 0x32;
	edid->Descriptor3[8] = 0x35;
	edid->Descriptor3[9] = 0x30;
	edid->Descriptor3[10] = 0x0A;
	edid->Descriptor3[11] = 0x20;
	edid->Descriptor3[12] = 0x20;
	edid->Descriptor3[13] = 0x20;
	edid->Descriptor3[14] = 0x20;
	edid->Descriptor3[15] = 0x20;
	edid->Descriptor3[16] = 0x20;
	edid->Descriptor3[17] = 0x20;

	/* 00 00 00 ff 00 30 31 30 4e 44 52 46 32 36 30 33 38 0a */
	edid->Descriptor4[0] = 0x00;
	edid->Descriptor4[1] = 0x00;
	edid->Descriptor4[2] = 0x00;
	edid->Descriptor4[3] = 0xFF;
	edid->Descriptor4[4] = 0x00;
	edid->Descriptor4[5] = 0x30;
	edid->Descriptor4[6] = 0x31;
	edid->Descriptor4[7] = 0x30;
	edid->Descriptor4[8] = 0x4E;
	edid->Descriptor4[9] = 0x44;
	edid->Descriptor4[10] = 0x52;
	edid->Descriptor4[11] = 0x46;
	edid->Descriptor4[12] = 0x32;
	edid->Descriptor4[13] = 0x36;
	edid->Descriptor4[14] = 0x30;
	edid->Descriptor4[15] = 0x33;
	edid->Descriptor4[16] = 0x38;
	edid->Descriptor4[17] = 0x0A;

	edid->NumberOfExtensions = 0;

	edid->Checksum = 0xA2;

	return edid;
}

BYTE* rdpEdidToBuffer(EDID* edid)
{
	int i;
	wStream* s;
	BYTE* data;
	int length = 128;

	data = (BYTE*) malloc(length);
	ZeroMemory(data, length);

	s = Stream_New(data, length);

	Stream_Write(s, &(edid->Header), 8);
	Stream_Write_UINT16(s, edid->ManufacturerId);
	Stream_Write_UINT16(s, edid->ManufacturerProductCode);
	Stream_Write_UINT32(s, edid->ManufacturerSerialNumber);
	Stream_Write_UINT8(s, edid->WeekOfManufacture);
	Stream_Write_UINT8(s, edid->YearOfManufacture);
	Stream_Write_UINT8(s, edid->EdidVersion);
	Stream_Write_UINT8(s, edid->EdidRevision);
	Stream_Write(s, &(edid->DisplayParameters), 4);
	Stream_Write(s, &(edid->ChromacityCoordinates), 10);
	Stream_Write(s, &(edid->BitmapTiming), 4);
	Stream_Write(s, &(edid->StandardTiming), 16);
	Stream_Write(s, &(edid->Descriptor1), 18);
	Stream_Write(s, &(edid->Descriptor2), 18);
	Stream_Write(s, &(edid->Descriptor3), 18);
	Stream_Write(s, &(edid->Descriptor4), 18);
	Stream_Write_UINT8(s, edid->NumberOfExtensions);

	edid->Checksum = 0;
	Stream_Write_UINT8(s, edid->Checksum);

	for (i = 0; i < length; i++)
		edid->Checksum = (edid->Checksum + data[i]) % 256;

	edid->Checksum = 256 - edid->Checksum;

	Stream_Rewind(s, 1);
	Stream_Write_UINT8(s, edid->Checksum);

	Stream_Free(s, FALSE);

	return data;
}

void rdpSetOutputEdid(RROutputPtr output, EDID* edid)
{
	int length;
	BYTE* buffer;
	ScreenPtr pScreen;
	rdpRandRInfoPtr randr;

	pScreen = output->pScreen;
	randr = rdpGetRandRFromScreen(pScreen);

	length = 128;
	buffer = rdpEdidToBuffer(edid);

	if (!randr->edidAtom)
	{
		randr->edidAtom = MakeAtom(EDID_ATOM_NAME, sizeof(EDID_ATOM_NAME) - 1, TRUE);
	}

	if (length)
	{
		RRChangeOutputProperty(output, randr->edidAtom, XA_INTEGER, 8,
				PropModeReplace, length, buffer, FALSE, TRUE);
	}
	else
	{
		RRDeleteOutputProperty(output, randr->edidAtom);
	}

	free(buffer);
}

static const char* g_MonitorsXmlFmt =
"<monitors version=\"1\">\n"
"  <configuration>\n"
"      <clone>no</clone>\n"
"      <output name=\"%s\">\n"
"          <vendor>VSC</vendor>\n"
"          <product>0x452d</product>\n"
"          <serial>0x01010101</serial>\n"
"          <width>%d</width>\n"
"          <height>%d</height>\n"
"          <rate>%d</rate>\n"
"          <x>%d</x>\n"
"          <y>%d</y>\n"
"          <rotation>normal</rotation>\n"
"          <reflect_x>no</reflect_x>\n"
"          <reflect_y>no</reflect_y>\n"
"          <primary>yes</primary>\n"
"      </output>\n"
"  </configuration>\n"
"</monitors>\n";

int rdpWriteGnomeMonitorsConfiguration(int width, int height)
{
	FILE* fp;
	int length;
	char monitors_xml[2048];
	char* monitors_xml_path;

	sprintf(monitors_xml, g_MonitorsXmlFmt, "RDP-0", width, height, 60, 0, 0);

	length = strlen(monitors_xml);

	monitors_xml_path = GetKnownSubPath(KNOWN_PATH_XDG_CONFIG_HOME, "monitors.xml");

	if (PathFileExistsA(monitors_xml_path))
	{
		DeleteFileA(monitors_xml_path);
	}

	fp = fopen(monitors_xml_path, "w+b");

	if (fp)
	{
		fwrite((void*) monitors_xml, length, 1, fp);
		fflush(fp);
		fclose(fp);
	}

	free(monitors_xml_path);

	return 0;
}

int rdpProbeModes(ScreenPtr pScreen)
{
	int k;
	int index;
	int shmmax;
	int width;
	int height;
	char name[64];
	xRRModeInfo modeInfo;

	rdpRandRInfoPtr randr = rdpGetRandRFromScreen(pScreen);

	if (!randr)
		return -1;

	if (!randr->modeCount)
	{
		randr->modeCount = 64;
		randr->modes = (RRModePtr*) malloc(sizeof(RRModePtr) * randr->modeCount);
		ZeroMemory(randr->modes, sizeof(RRModePtr) * randr->modeCount);
	}

	index = 0;
	shmmax = get_max_shared_memory_segment_size();

	ZeroMemory(&modeInfo, sizeof(xRRModeInfo));

	for (k = 0; k < sizeof(g_StandardSizes) / sizeof(UINT32); k++)
	{
		width = SCREEN_SIZE_WIDTH(g_StandardSizes[k]);
		height = SCREEN_SIZE_HEIGHT(g_StandardSizes[k]);

		if ((width > MAX_SCREEN_SIZE_WIDTH) || (height > MAX_SCREEN_SIZE_HEIGHT))
			continue; /* screen size is too large */

		if (shmmax > 0)
		{
			if ((width * height * 4) > shmmax)
				continue; /* required buffer size is too large */
		}

		rdpGtfMode(&modeInfo, width, height, 60.0, 0, 0);

		sprintf(name, "%dx%d", modeInfo.width, modeInfo.height);
		modeInfo.nameLength = strlen(name);

		randr->modes[index] = RRModeGet(&modeInfo, name);

		index++;
	}

	randr->numModes = index;

	rdpModeSelect(pScreen, randr->width, randr->height);

	return 0;
}

int rdpModeInfoCompare(xRRModeInfo* pModeInfo1, xRRModeInfo* pModeInfo2)
{
	int size1, size2;

	size1 = pModeInfo1->width * pModeInfo1->height;
	size2 = pModeInfo2->width * pModeInfo2->height;

	if (size1 < size2)
		return -1;
	else if (size1 > size2)
		return 1;
	else
		return 0;
}

int rdpModeAdd(ScreenPtr pScreen, int width, int height)
{
	int index;
	char name[64];
	RRModePtr mode;
	RROutputPtr output;
	xRRModeInfo modeInfo;
	rdpRandRInfoPtr randr;

	randr = rdpGetRandRFromScreen(pScreen);

	if (!randr)
		return -1;

	ZeroMemory(&modeInfo, sizeof(xRRModeInfo));

	rdpGtfMode(&modeInfo, width, height, 60.0, 0, 0);

	sprintf(name, "%dx%d", modeInfo.width, modeInfo.height);
	modeInfo.nameLength = strlen(name);

	for (index = 0; index < randr->numModes; index++)
	{
		if (rdpModeInfoCompare(&(randr->modes[index]->mode), &modeInfo) < 0)
			break;
	}

	mode = RRModeGet(&modeInfo, name);

	MoveMemory(&(randr->modes[index + 1]), &(randr->modes[index]), (randr->numModes - index) * sizeof(RRModePtr));
	randr->numModes++;

	randr->modes[index] = mode;

	output = RRFirstOutput(pScreen);

	if (output)
	{
		if (!RROutputSetModes(output, randr->modes, randr->numModes, 0))
			return -1;
	}

	return 0;
}

int rdpModeSelect(ScreenPtr pScreen, int width, int height)
{
	int index;
	BOOL found = FALSE;
	rdpRandRInfoPtr randr;
	xRRModeInfo* pModeInfo;

	randr = rdpGetRandRFromScreen(pScreen);

	if (!randr)
		return -1;

	for (index = 0; index < randr->numModes; index++)
	{
		pModeInfo = &(randr->modes[index]->mode);

		if ((pModeInfo->width == width) && (pModeInfo->height == height))
		{
			randr->mode = randr->modes[index];
			found = TRUE;
			break;
		}
	}

	if (!found)
	{
		if (rdpModeAdd(pScreen, width, height) < 0)
			return -1;

		return rdpModeSelect(pScreen, width, height);
	}

	if (!found && !randr->mode)
		randr->mode = randr->modes[0];

	randr->width = randr->mode->mode.width;
	randr->height = randr->mode->mode.height;

	return 0;
}

/*
 * gtf.c  Generate mode timings using the GTF Timing Standard
 *
 * gcc gtf.c -o gtf -lm -Wall
 *
 * Copyright (c) 2001, Andy Ritger  aritger@nvidia.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * o Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * o Neither the name of NVIDIA nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * This program is based on the Generalized Timing Formula(GTF TM)
 * Standard Version: 1.0, Revision: 1.0
 *
 * The GTF Document contains the following Copyright information:
 *
 * Copyright (c) 1994, 1995, 1996 - Video Electronics Standards
 * Association. Duplication of this document within VESA member
 * companies for review purposes is permitted. All other rights
 * reserved.
 *
 * While every precaution has been taken in the preparation
 * of this standard, the Video Electronics Standards Association and
 * its contributors assume no responsibility for errors or omissions,
 * and make no warranties, expressed or implied, of functionality
 * of suitability for any purpose. The sample code contained within
 * this standard may be used without restriction.
 *
 *
 *
 * The GTF EXCEL(TM) SPREADSHEET, a sample (and the definitive)
 * implementation of the GTF Timing Standard, is available at:
 *
 * ftp://ftp.vesa.org/pub/GTF/GTF_V1R1.xls
 */

/* Ruthlessly converted to server code by Adam Jackson <ajax@redhat.com> */

/* this code has been adapted from xf86gtf.c */

#define MARGIN_PERCENT    1.8   /* % of active vertical image                */
#define CELL_GRAN         8.0   /* assumed character cell granularity        */
#define MIN_PORCH         1     /* minimum front porch                       */
#define V_SYNC_RQD        3     /* width of vsync in lines                   */
#define H_SYNC_PERCENT    8.0   /* width of hsync as % of total line         */
#define MIN_VSYNC_PLUS_BP 550.0 /* min time of vsync + back porch (microsec) */
#define M                 600.0 /* blanking formula gradient                 */
#define C                 40.0  /* blanking formula offset                   */
#define K                 128.0 /* blanking formula scaling factor           */
#define J                 20.0  /* blanking formula scaling factor           */

/* C' and M' are part of the Blanking Duty Cycle computation */

#define C_PRIME           (((C - J) * K/256.0) + J)
#define M_PRIME           (K/256.0 * M)

int rdpGtfMode(xRRModeInfo* modeInfo, int h_pixels, int v_lines, float freq, int interlaced, int margins)
{
	float h_pixels_rnd;
	float v_lines_rnd;
	float v_field_rate_rqd;
	float top_margin;
	float bottom_margin;
	float interlace;
	float h_period_est;
	float vsync_plus_bp;
	float v_back_porch;
	float total_v_lines;
	float v_field_rate_est;
	float h_period;
	float v_field_rate;
	float v_frame_rate;
	float left_margin;
	float right_margin;
	float total_active_pixels;
	float ideal_duty_cycle;
	float h_blank;
	float total_pixels;
	float pixel_freq;
	float h_freq;

	float h_sync;
	float h_front_porch;
	float v_odd_front_porch_lines;

	/*  1. In order to give correct results, the number of horizontal
	 *  pixels requested is first processed to ensure that it is divisible
	 *  by the character size, by rounding it to the nearest character
	 *  cell boundary:
	 *
	 *  [H PIXELS RND] = ((ROUND([H PIXELS]/[CELL GRAN RND],0))*[CELLGRAN RND])
	 */

	h_pixels_rnd = rint((float) h_pixels / CELL_GRAN) * CELL_GRAN;

	/*  2. If interlace is requested, the number of vertical lines assumed
	 *  by the calculation must be halved, as the computation calculates
	 *  the number of vertical lines per field. In either case, the
	 *  number of lines is rounded to the nearest integer.
	 *
	 *  [V LINES RND] = IF([INT RQD?]="y", ROUND([V LINES]/2,0),
	 *                                     ROUND([V LINES],0))
	 */

	v_lines_rnd = interlaced ? rint((float) v_lines) / 2.0 : rint((float) v_lines);

	/*  3. Find the frame rate required:
	 *
	 *  [V FIELD RATE RQD] = IF([INT RQD?]="y", [I/P FREQ RQD]*2,
	 *                                          [I/P FREQ RQD])
	 */

	v_field_rate_rqd = interlaced ? (freq * 2.0) : (freq);

	/*  4. Find number of lines in Top margin:
	 *
	 *  [TOP MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
	 *          ROUND(([MARGIN%]/100*[V LINES RND]),0),
	 *          0)
	 */

	top_margin = margins ? rint(MARGIN_PERCENT / 100.0 * v_lines_rnd) : (0.0);

	/*  5. Find number of lines in Bottom margin:
	 *
	 *  [BOT MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
	 *          ROUND(([MARGIN%]/100*[V LINES RND]),0),
	 *          0)
	 */

	bottom_margin = margins ? rint(MARGIN_PERCENT / 100.0 * v_lines_rnd) : (0.0);

	/*  6. If interlace is required, then set variable [INTERLACE]=0.5:
	 *
	 *  [INTERLACE]=(IF([INT RQD?]="y",0.5,0))
	 */

	interlace = interlaced ? 0.5 : 0.0;

	/*  7. Estimate the Horizontal period
	 *
	 *  [H PERIOD EST] = ((1/[V FIELD RATE RQD]) - [MIN VSYNC+BP]/1000000) /
	 *                    ([V LINES RND] + (2*[TOP MARGIN (LINES)]) +
	 *                     [MIN PORCH RND]+[INTERLACE]) * 1000000
	 */

	h_period_est = (((1.0 / v_field_rate_rqd) - (MIN_VSYNC_PLUS_BP / 1000000.0))
			/ (v_lines_rnd + (2 * top_margin) + MIN_PORCH + interlace)
			* 1000000.0);

	/*  8. Find the number of lines in V sync + back porch:
	 *
	 *  [V SYNC+BP] = ROUND(([MIN VSYNC+BP]/[H PERIOD EST]),0)
	 */

	vsync_plus_bp = rint(MIN_VSYNC_PLUS_BP / h_period_est);

	/*  9. Find the number of lines in V back porch alone:
	 *
	 *  [V BACK PORCH] = [V SYNC+BP] - [V SYNC RND]
	 *
	 *  XXX is "[V SYNC RND]" a typo? should be [V SYNC RQD]?
	 */

	v_back_porch = vsync_plus_bp - V_SYNC_RQD;

	/*  10. Find the total number of lines in Vertical field period:
	 *
	 *  [TOTAL V LINES] = [V LINES RND] + [TOP MARGIN (LINES)] +
	 *                    [BOT MARGIN (LINES)] + [V SYNC+BP] + [INTERLACE] +
	 *                    [MIN PORCH RND]
	 */

	total_v_lines = v_lines_rnd + top_margin + bottom_margin + vsync_plus_bp +
			interlace + MIN_PORCH;

	/*  11. Estimate the Vertical field frequency:
	 *
	 *  [V FIELD RATE EST] = 1 / [H PERIOD EST] / [TOTAL V LINES] * 1000000
	 */

	v_field_rate_est = 1.0 / h_period_est / total_v_lines * 1000000.0;

	/*  12. Find the actual horizontal period:
	 *
	 *  [H PERIOD] = [H PERIOD EST] / ([V FIELD RATE RQD] / [V FIELD RATE EST])
	 */

	h_period = h_period_est / (v_field_rate_rqd / v_field_rate_est);

	/*  13. Find the actual Vertical field frequency:
	 *
	 *  [V FIELD RATE] = 1 / [H PERIOD] / [TOTAL V LINES] * 1000000
	 */

	v_field_rate = 1.0 / h_period / total_v_lines * 1000000.0;

	/*  14. Find the Vertical frame frequency:
	 *
	 *  [V FRAME RATE] = (IF([INT RQD?]="y", [V FIELD RATE]/2, [V FIELD RATE]))
	 */

	v_frame_rate = interlaced ? v_field_rate / 2.0 : v_field_rate;

	/*  15. Find number of pixels in left margin:
	 *
	 *  [LEFT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
	 *          (ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
	 *                   [CELL GRAN RND]),0)) * [CELL GRAN RND],
	 *          0))
	 */

	left_margin = margins ?
			rint(h_pixels_rnd * MARGIN_PERCENT / 100.0 / CELL_GRAN) * CELL_GRAN :
			0.0;

	/*  16. Find number of pixels in right margin:
	 *
	 *  [RIGHT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
	 *          (ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
	 *                   [CELL GRAN RND]),0)) * [CELL GRAN RND],
	 *          0))
	 */

	right_margin = margins ?
			rint(h_pixels_rnd * MARGIN_PERCENT / 100.0 / CELL_GRAN) * CELL_GRAN :
			0.0;

	/*  17. Find total number of active pixels in image and left and right
	 *  margins:
	 *
	 *  [TOTAL ACTIVE PIXELS] = [H PIXELS RND] + [LEFT MARGIN (PIXELS)] +
	 *                          [RIGHT MARGIN (PIXELS)]
	 */

	total_active_pixels = h_pixels_rnd + left_margin + right_margin;

	/*  18. Find the ideal blanking duty cycle from the blanking duty cycle
	 *  equation:
	 *
	 *  [IDEAL DUTY CYCLE] = [C'] - ([M']*[H PERIOD]/1000)
	 */

	ideal_duty_cycle = C_PRIME - (M_PRIME * h_period / 1000.0);

	/*  19. Find the number of pixels in the blanking time to the nearest
	 *  double character cell:
	 *
	 *  [H BLANK (PIXELS)] = (ROUND(([TOTAL ACTIVE PIXELS] *
	 *                               [IDEAL DUTY CYCLE] /
	 *                               (100-[IDEAL DUTY CYCLE]) /
	 *                               (2*[CELL GRAN RND])), 0))
	 *                       * (2*[CELL GRAN RND])
	 */

	h_blank = rint(total_active_pixels *
			ideal_duty_cycle /
			(100.0 - ideal_duty_cycle) /
			(2.0 * CELL_GRAN)) * (2.0 * CELL_GRAN);

	/*  20. Find total number of pixels:
	 *
	 *  [TOTAL PIXELS] = [TOTAL ACTIVE PIXELS] + [H BLANK (PIXELS)]
	 */

	total_pixels = total_active_pixels + h_blank;

	/*  21. Find pixel clock frequency:
	 *
	 *  [PIXEL FREQ] = [TOTAL PIXELS] / [H PERIOD]
	 */

	pixel_freq = total_pixels / h_period;

	/*  22. Find horizontal frequency:
	 *
	 *  [H FREQ] = 1000 / [H PERIOD]
	 */

	h_freq = 1000.0 / h_period;

	/* Stage 1 computations are now complete; I should really pass
	 * the results to another function and do the Stage 2
	 * computations, but I only need a few more values so I'll just
	 * append the computations here for now */

	/*  17. Find the number of pixels in the horizontal sync period:
	 *
	 *  [H SYNC (PIXELS)] =(ROUND(([H SYNC%] / 100 * [TOTAL PIXELS] /
	 *                             [CELL GRAN RND]),0))*[CELL GRAN RND]
	 */

	h_sync = rint(H_SYNC_PERCENT / 100.0 * total_pixels / CELL_GRAN) * CELL_GRAN;

	/*  18. Find the number of pixels in the horizontal front porch period:
	 *
	 *  [H FRONT PORCH (PIXELS)] = ([H BLANK (PIXELS)]/2)-[H SYNC (PIXELS)]
	 */

	h_front_porch = (h_blank / 2.0) - h_sync;

	/*  36. Find the number of lines in the odd front porch period:
	 *
	 *  [V ODD FRONT PORCH(LINES)]=([MIN PORCH RND]+[INTERLACE])
	 */

	v_odd_front_porch_lines = MIN_PORCH + interlace;

	/* finally, pack the results in the mode struct */

	modeInfo->width = (int) h_pixels;
	//modeInfo->width = (int) (h_pixels_rnd);
	modeInfo->hSyncStart = (int) (h_pixels_rnd + h_front_porch);
	modeInfo->hSyncEnd = (int) (h_pixels_rnd + h_front_porch + h_sync);
	modeInfo->hTotal = (int) (total_pixels);

	modeInfo->height = (int) v_lines;
	//modeInfo->height = (int) (v_lines_rnd);
	modeInfo->vSyncStart = (int) (v_lines_rnd + v_odd_front_porch_lines);
	modeInfo->vSyncEnd = (int) (v_lines_rnd + v_odd_front_porch_lines + V_SYNC_RQD);
	modeInfo->vTotal = (int) (total_v_lines);

	/* DotClock = RefreshRate * HTotal * VTotal */
	modeInfo->dotClock = freq * modeInfo->hTotal * modeInfo->vTotal;

	modeInfo->hSkew = 0;

	return 0;
}
