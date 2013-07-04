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

#include "xrdp.h"
#include "log.h"

#include <winpr/crt.h>
#include <winpr/thread.h>

#include <freerdp/listener.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/signal.h>

void xrdp_peer_accepted(freerdp_listener* instance, freerdp_peer* client)
{
	HANDLE thread;
	xrdpProcess* process;
	printf("xrdp_peer_accepted\n");

	process = xrdp_process_create_ex(NULL, 0, (void*) client);

	thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) xrdp_process_main_thread, client, 0, NULL);
}

xrdpListener* xrdp_listen_create(void)
{
	freerdp_listener* listener;

	listener = freerdp_listener_new();
	listener->PeerAccepted = xrdp_peer_accepted;

	return (xrdpListener*) listener;
}

void xrdp_listen_delete(xrdpListener* self)
{
	freerdp_listener_free((freerdp_listener*) self);
}

int xrdp_listen_main_loop(xrdpListener* self)
{
	int i;
	int fds;
	int max_fds;
	int robjc;
	int wobjc;
	long sync_obj;
	long term_obj;
	long robjs[32];
	int itimeout;
	int rcount;
	void* rfds[32];
	fd_set rfds_set;
	freerdp_listener* listener;

	ZeroMemory(rfds, sizeof(rfds));
	listener = (freerdp_listener*) self;

	listener->Open(listener, NULL, 3389);

	term_obj = g_get_term_event();
	sync_obj = g_get_sync_event();

	while (1)
	{
		rcount = 0;

		robjc = 0;
		wobjc = 0;
		itimeout = -1;

		robjs[robjc++] = term_obj;
		robjs[robjc++] = sync_obj;

		if (listener->GetFileDescriptor(listener, rfds, &rcount) != TRUE)
		{
			fprintf(stderr, "Failed to get FreeRDP file descriptor\n");
			break;
		}

		max_fds = 0;
		FD_ZERO(&rfds_set);

		for (i = 0; i < rcount; i++)
		{
			fds = (int)(long)(rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		for (i = 0; i < robjc; i++)
		{
			fds = robjs[i];

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		if (max_fds == 0)
			break;

		if (select(max_fds + 1, &rfds_set, NULL, NULL, NULL) == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				fprintf(stderr, "select failed\n");
				break;
			}
		}

		if (listener->CheckFileDescriptor(listener) != TRUE)
		{
			fprintf(stderr, "Failed to check FreeRDP file descriptor\n");
			break;
		}

		if (g_is_wait_obj_set(term_obj))
		{
			break;
		}

		if (g_is_wait_obj_set(sync_obj))
		{
			g_reset_wait_obj(sync_obj);
			g_process_waiting_function();
		}
	}

	listener->Close(listener);

	return 0;
}

