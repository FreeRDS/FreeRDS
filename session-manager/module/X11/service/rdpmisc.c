/*
Copyright 2005-2012 Jay Sorg

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

the rest

 */

#include "rdp.h"

#include <sys/un.h>

Bool noFontCacheExtension = 1;

/* print a time-stamped message to the log file (stderr). */
void rdpLog(char *format, ...)
{
	va_list args;
	char buf[256];
	time_t clock;

	va_start(args, format);
	time(&clock);
	strftime(buf, 255, "%d/%m/%y %T ", localtime(&clock));
	fprintf(stderr, "%s", buf);
	vfprintf(stderr, format, args);
	fflush(stderr);
	va_end(args);
}

int rdpBitsPerPixel(int depth)
{
	if (depth == 1)
	{
		return 1;
	}
	else if (depth <= 8)
	{
		return 8;
	}
	else if (depth <= 16)
	{
		return 16;
	}
	else
	{
		return 32;
	}
}

void rdpClientStateChange(CallbackListPtr *cbl, pointer myData, pointer clt)
{
	dispatchException &= ~DE_RESET; /* hack - force server not to reset */
}

/* DPMS */

Bool DPMSSupported(void)
{
    return FALSE;
}

int DPMSSet(ClientPtr client, int level)
{
    return Success;
}

void AddOtherInputDevices(void)
{
}

void OpenInputDevice(DeviceIntPtr dev, ClientPtr client, int *status)
{
}

int SetDeviceValuators(register ClientPtr client, DeviceIntPtr dev,
		int *valuators, int first_valuator, int num_valuators)
{
	return BadMatch;
}

int SetDeviceMode(register ClientPtr client, DeviceIntPtr dev, int mode)
{
	return BadMatch;
}

int ChangeKeyboardDevice(DeviceIntPtr old_dev, DeviceIntPtr new_dev)
{
	return BadMatch;
}

int ChangeDeviceControl(register ClientPtr client, DeviceIntPtr dev, void *control)
{
	return BadMatch;
}

int ChangePointerDevice(DeviceIntPtr old_dev, DeviceIntPtr new_dev,
		unsigned char x, unsigned char y)
{
	return BadMatch;
}

void CloseInputDevice(DeviceIntPtr d, ClientPtr client)
{
}

/*
  stub for XpClient* functions.
 */

Bool XpClientIsBitmapClient(ClientPtr client)
{
	return 1;
}

Bool XpClientIsPrintClient(ClientPtr client, FontPathElementPtr fpe)
{
	return 0;
}

int PrinterOptions(int argc, char **argv, int i)
{
	return i;
}

void PrinterInitOutput(ScreenInfo *pScreenInfo, int argc, char **argv)
{
}

void PrinterUseMsg(void)
{
}

void PrinterInitGlobals(void)
{
}

void FontCacheExtensionInit(INITARGS)
{
}

void RegionAroundSegs(RegionPtr reg, xSegment *segs, int nseg)
{
	int index;
	BoxRec box;
	RegionRec treg;

	index = 0;

	while (index < nseg)
	{
		if (segs[index].x1 < segs[index].x2)
		{
			box.x1 = segs[index].x1;
			box.x2 = segs[index].x2;
		}
		else
		{
			box.x1 = segs[index].x2;
			box.x2 = segs[index].x1;
		}

		box.x2++;

		if (segs[index].y1 < segs[index].y2)
		{
			box.y1 = segs[index].y1;
			box.y2 = segs[index].y2;
		}
		else
		{
			box.y1 = segs[index].y2;
			box.y2 = segs[index].y1;
		}

		box.y2++;
		RegionInit(&treg, &box, 0);
		RegionUnion(reg, reg, &treg);
		RegionUninit(&treg);
		index++;
	}
}
