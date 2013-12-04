/**
 * FreeRDS: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bernhard.miklautz@thincast.com>
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

/**
 * Short description
 * optional longer/detailed description
 *
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

#include "freerds.h"

#include <winpr/pipe.h>
#include <winpr/path.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/stream.h>
#include <winpr/sspicli.h>
#include <winpr/environment.h>

#include <freerdp/freerdp.h>

void* freerds_client_thread(void* arg)
{
	int fps;
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE PackTimer;
	LARGE_INTEGER due;
	rdsBackendConnector* connector = (rdsBackendConnector*) arg;

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
			if (freerds_transport_receive((rdsBackend *)connector) < 0)
				break;
		}

		if (status == WAIT_OBJECT_0)
		{
			freerds_message_server_queue_pack(connector);
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

int freerds_client_get_event_handles(rdsBackend* backend, HANDLE* events, DWORD* nCount)
{

	rdsBackendConnector *connector = (rdsBackendConnector *)backend;
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

int freerds_client_check_event_handles(rdsBackend* backend)
{
	int status = 0;

	rdsBackendConnector *connector = (rdsBackendConnector *)backend;
	if (!connector)
		return 0;

	while (WaitForSingleObject(MessageQueue_Event(connector->ServerQueue), 0) == WAIT_OBJECT_0)
	{
		status = freerds_message_server_queue_process_pending_messages(connector);
	}

	return status;
}
