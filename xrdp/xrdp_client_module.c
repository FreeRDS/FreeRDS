/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xrdp.h"

#include <winpr/pipe.h>
#include <winpr/path.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/stream.h>
#include <winpr/sspicli.h>
#include <winpr/environment.h>

#include <freerdp/freerdp.h>

void* xrdp_client_thread(void* arg)
{
	int fps;
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE PackTimer;
	LARGE_INTEGER due;
	rdsModuleConnector* connector = (rdsModuleConnector*) arg;

	fps = connector->fps;
	PackTimer = CreateWaitableTimer(NULL, TRUE, NULL);

	due.QuadPart = 0;
	SetWaitableTimer(PackTimer, &due, 1000 / fps, NULL, NULL, 0);

	nCount = 0;
	events[nCount++] = PackTimer;
	events[nCount++] = connector->StopEvent;
	events[nCount++] = connector->hClientPipe;

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(connector->StopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(connector->hClientPipe, 0) == WAIT_OBJECT_0)
		{
			if (freerds_transport_receive(connector) < 0)
				break;
		}

		if (status == WAIT_OBJECT_0)
		{
			xrdp_message_server_queue_pack(connector);
		}

		if (connector->fps != fps)
		{
			fps = connector->fps;
			due.QuadPart = 0;
			SetWaitableTimer(PackTimer, &due, 1000 / fps, NULL, NULL, 0);
		}
	}

	CloseHandle(PackTimer);

	return NULL;
}


int xrdp_client_get_event_handles(rdsModuleConnector* connector, HANDLE* events, DWORD* nCount)
{
	if (connector)
	{
		if (connector->ServerQueue)
		{
			events[*nCount] = MessageQueue_Event(connector->ServerQueue);
			(*nCount)++;
		}
	}

	return 0;
}

int xrdp_client_check_event_handles(rdsModuleConnector* connector)
{
	int status = 0;


	if (!connector)
		return 0;

	while (WaitForSingleObject(MessageQueue_Event(connector->ServerQueue), 0) == WAIT_OBJECT_0)
	{
		status = xrdp_message_server_queue_process_pending_messages(connector);
	}

	return status;
}
