/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include <winpr/crt.h>
#include <winpr/thread.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/signal.h>

void freerds_peer_context_new(freerdp_peer* client, rdsConnection* context)
{
	FILE* fp = NULL;
	rdpSettings* settings = client->settings;

	settings->OsMajorType = OSMAJORTYPE_UNIX;
	settings->OsMinorType = OSMINORTYPE_PSEUDO_XSERVER;
	settings->ColorDepth = 32;
	settings->RemoteFxCodec = TRUE;
	settings->BitmapCacheV3Enabled = TRUE;
	settings->FrameMarkerCommandEnabled = TRUE;
	settings->SurfaceFrameMarkerEnabled = TRUE;

	settings->RdpKeyFile = strdup("freerds.pem");

	fp = fopen(settings->RdpKeyFile, "rb");

	if (!fp)
	{
		/*
		 * This is the first time FreeRDS has been executed and
		 * the RSA keys do not exist.  Go ahead and create them
		 * using the OpenSSL command line utilities.
		 */

		char command[256];

		sprintf(command, "openssl genrsa -out %s 1024", settings->RdpKeyFile);
		system(command);
	}
	else
	{
		fclose(fp);
	}

	context->id = app_context_get_connectionid();
	context->settings = settings;

	context->bytesPerPixel = 4;

	context->FrameList = ListDictionary_New(TRUE);

	context->client = client;
	context->vcm = WTSOpenServerA((LPSTR) client->context);
}

void freerds_peer_context_free(freerdp_peer* client, rdsConnection* context)
{
	freerds_bitmap_encoder_free(context->encoder);

	ListDictionary_Free(context->FrameList);

	WTSCloseServer((HANDLE) context->vcm);
}

void freerds_peer_accepted(freerdp_listener* instance, freerdp_peer* client)
{
	rdsConnection* connection;

	client->ContextSize = sizeof(rdsConnection);
	client->ContextNew = (psPeerContextNew) freerds_peer_context_new;
	client->ContextFree = (psPeerContextFree) freerds_peer_context_free;
	freerdp_peer_context_new(client);

	connection = (rdsConnection*) client->context;

	connection->TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	connection->notifications = MessageQueue_New(NULL);

	connection->Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) freerds_connection_main_thread, client, 0, NULL);
}

rdsListener* freerds_listener_new(void)
{
	freerdp_listener* listener;

	listener = freerdp_listener_new();
	listener->PeerAccepted = freerds_peer_accepted;

	return (rdsListener*) listener;
}

void freerds_listener_free(rdsListener* self)
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

