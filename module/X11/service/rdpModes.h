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

#ifndef FREERDS_X11RDP_MODES_H
#define FREERDS_X11RDP_MODES_H

struct _EDID
{
	BYTE Header[8];
	UINT16 ManufacturerId;
	UINT16 ManufacturerProductCode;
	UINT32 ManufacturerSerialNumber;
	BYTE WeekOfManufacture;
	BYTE YearOfManufacture;
	BYTE EdidVersion;
	BYTE EdidRevision;
	BYTE DisplayParameters[4];
	BYTE ChromacityCoordinates[10];
	BYTE BitmapTiming[4];
	BYTE StandardTiming[16];
	BYTE Descriptor1[18];
	BYTE Descriptor2[18];
	BYTE Descriptor3[18];
	BYTE Descriptor4[18];
	BYTE NumberOfExtensions;
	BYTE Checksum;
};
typedef struct _EDID EDID;

#define EDID_ATOM_NAME		"EDID"

#define MAX_SCREEN_SIZE_WIDTH	1920
#define MAX_SCREEN_SIZE_HEIGHT	1080

#define SCREEN_SIZE_WIDTH(_size) ((_size >> 16) & 0xFFFF)
#define SCREEN_SIZE_HEIGHT(_size) ((_size) & 0xFFFF)

EDID* rdpConstructScreenEdid(ScreenPtr pScreen);
BYTE* rdpEdidToBuffer(EDID* edid);

void rdpSetOutputEdid(RROutputPtr output, EDID* edid);
int rdpWriteMonitorConfig(ScreenPtr pScreen, int width, int height);

int rdpProbeModes(ScreenPtr pScreen);
int rdpModeSelect(ScreenPtr pScreen, int width, int height);

int rdpGtfMode(xRRModeInfo* modeInfo, int h_pixels, int v_lines, float freq, int interlaced, int margins);

#endif /* FREERDS_X11RDP_MODES_H */
