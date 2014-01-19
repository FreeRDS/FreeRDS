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
#include "rdpScreen.h"

#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>

extern rdpScreenInfoRec g_rdpScreen;

int get_min_shared_memory_segment_size(void)
{
#ifdef _GNU_SOURCE
	struct shminfo info;

	if ((shmctl(0, IPC_INFO, (struct shmid_ds*)(void*)&info)) == -1)
		return -1;

	return info.shmmin;
#else
	return -1;
#endif
}

int get_max_shared_memory_segment_size(void)
{
#ifdef _GNU_SOURCE
	struct shminfo info;

	if ((shmctl(0, IPC_INFO, (struct shmid_ds*)(void*)&info)) == -1)
		return -1;

	return info.shmmax;
#else
	return -1;
#endif
}

int rdpScreenBitsPerPixel(int depth)
{
	if (depth == 1)
		return 1;
	else if (depth <= 8)
		return 8;
	else if (depth <= 16)
		return 16;
	else
		return 32;
}

int rdpScreenPixelToMM(int pixels)
{
	return ((int)(((((double)(pixels)) / g_rdpScreen.dpi) * 25.4 + 0.5)));
}

int rdpScreenFrameBufferAlloc()
{
	int shmmin;

	shmmin = get_min_shared_memory_segment_size();

	g_rdpScreen.bitsPerPixel = rdpScreenBitsPerPixel(g_rdpScreen.depth);
	g_rdpScreen.bytesPerPixel = g_rdpScreen.bitsPerPixel / 8;

	g_rdpScreen.scanline = g_rdpScreen.width * g_rdpScreen.bytesPerPixel;
	g_rdpScreen.scanline += (g_rdpScreen.scanline % 16);

	g_rdpScreen.sizeInBytes = (g_rdpScreen.scanline * g_rdpScreen.height);

	if (shmmin > 0)
	{
		if (g_rdpScreen.sizeInBytes < shmmin)
			g_rdpScreen.sizeInBytes = shmmin;
	}

	if (!g_rdpScreen.pfbMemory)
	{
		/* allocate shared memory segment */
		g_rdpScreen.segmentId = shmget(IPC_PRIVATE, g_rdpScreen.sizeInBytes,
				IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		/* attach the shared memory segment */
		g_rdpScreen.pfbMemory = (char*) shmat(g_rdpScreen.segmentId, 0, 0);

		if (!g_rdpScreen.pfbMemory)
		{
			ErrorF("rdpScreenInit pfbMemory malloc failed\n");
			return 0;
		}

		/* mark the shared memory segment for automatic deletion */
		shmctl(g_rdpScreen.segmentId, IPC_RMID, 0);

		ZeroMemory(g_rdpScreen.pfbMemory, g_rdpScreen.sizeInBytes);
	}

	return 0;
}

int rdpScreenFrameBufferFree()
{
	/* detach shared memory segment */
	shmdt(g_rdpScreen.pfbMemory);
	g_rdpScreen.pfbMemory = NULL;

	return 0;
}
