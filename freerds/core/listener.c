/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
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
 *
 * listen for incoming connection
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "freerds.h"

#include <winpr/crt.h>
#include <winpr/thread.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/signal.h>

void freerds_peer_accepted(freerdp_listener* instance, freerdp_peer* client)
{
	freerds_connection_create(client);
}

rdsListener* freerds_listener_create(void)
{
	freerdp_listener* listener;

	listener = freerdp_listener_new();
	listener->PeerAccepted = freerds_peer_accepted;

	return (rdsListener*) listener;
}

void freerds_listener_delete(rdsListener* self)
{
	freerdp_listener_free((freerdp_listener*) self);
}

int freerds_listener_main_loop(rdsListener* self)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[32];
	HANDLE TermEvent;
	freerdp_listener* listener;

	listener = (freerdp_listener*) self;

	listener->Open(listener, NULL, 3389);

	TermEvent = g_get_term_event();

	while (1)
	{
		nCount = 0;
		events[nCount++] = TermEvent;

		if (listener->GetEventHandles(listener, events, &nCount) < 0)
		{
			fprintf(stderr, "Failed to get FreeRDP file descriptor\n");
			break;
		}

		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(TermEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (listener->CheckFileDescriptor(listener) != TRUE)
		{
			fprintf(stderr, "Failed to check FreeRDP file descriptor\n");
			break;
		}
	}

	listener->Close(listener);

	return 0;
}

