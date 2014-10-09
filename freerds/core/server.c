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

#include "channels.h"

#include "freerds.h"

#include <winpr/crt.h>
#include <winpr/thread.h>
#include <winpr/interlocked.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/signal.h>

#include "rpc.h"

rdsServer* g_Server = NULL;

UINT32 freerds_server_get_connection_id(rdsServer* server)
{
	if (!server)
		server = g_Server;

	return InterlockedIncrement((volatile LONG*) &(server->connectionId));
}

void freerds_server_add_connection(rdsServer* server, rdsConnection* connection)
{
	if (!server)
		server = g_Server;

	ListDictionary_Add(server->connections, (void*) (UINT_PTR) connection->id, connection);
}

void freerds_server_remove_connection(rdsServer* server, UINT32 id)
{
	if (!server)
		server = g_Server;

	ListDictionary_Remove(server->connections, (void*) (UINT_PTR) id);
}

rdsConnection* freerds_server_get_connection(rdsServer* server, UINT32 id)
{
	if (!server)
		server = g_Server;

	return ListDictionary_GetItemValue(g_Server->connections, (void*) (UINT_PTR) id);
}

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

	context->id = freerds_server_get_connection_id(g_Server);
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

int freerds_server_main_loop(rdsServer* server)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[32];
	HANDLE TermEvent;
	HANDLE ChannelEvent;
	rdsChannelServer* channels;
	freerdp_listener* listener;

	listener = server->listener;
	channels = server->channels;

	server->rpc = pbrpc_server_new();
	pbrpc_server_start(server->rpc);

	listener->Open(listener, NULL, 3389);
	freerds_channel_server_open(server->channels);

	TermEvent = g_get_term_event();
	ChannelEvent = freerds_channel_server_get_event_handle(channels);

	while (1)
	{
		nCount = 0;
		events[nCount++] = TermEvent;
		events[nCount++] = ChannelEvent;

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

		if (!listener->CheckFileDescriptor(listener))
		{
			fprintf(stderr, "Failed to check FreeRDP file descriptor\n");
			break;
		}

		if (WaitForSingleObject(ChannelEvent, 0) == WAIT_OBJECT_0)
		{
			freerds_channel_server_check_socket(channels);
			break;
		}
	}

	if (server->rpc)
	{
		pbrpc_server_stop(server->rpc);
		pbrpc_server_free(server->rpc);
		server->rpc = NULL;
	}

	listener->Close(listener);

	return 0;
}

rdsServer* freerds_server_new(void)
{
	rdsServer* server;

	server = (rdsServer*) calloc(1, sizeof(rdsServer));

	if (server)
	{
		server->listener = freerdp_listener_new();
		server->listener->PeerAccepted = freerds_peer_accepted;

		server->channels = freerds_channel_server_new();

		server->connectionId = 0;
		server->connections = ListDictionary_New(TRUE);
	}

	return server;
}

void freerds_server_free(rdsServer* server)
{
	if (server->listener)
	{
		freerdp_listener_free(server->listener);
		server->listener = NULL;
	}

	if (server->channels)
	{
		freerds_channel_server_close(server->channels);
		freerds_channel_server_free(server->channels);
		server->channels = NULL;
	}

	if (server->connections)
	{
		ListDictionary_Free(server->connections);
		server->connections = NULL;
	}

	free(server);
}
