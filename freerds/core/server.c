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
#include <winpr/file.h>
#include <winpr/path.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/interlocked.h>

#include <winpr/tools/makecert.h>

#include <errno.h>

#ifndef _WIN32
#include <sys/select.h>
#include <sys/signal.h>
#endif

#include "rpc.h"

rdsServer* g_Server = NULL;

const char* makecert_argv[4] =
{
	"makecert",
	"-rdp",
	"-live",
	"-silent"
};

int makecert_argc = (sizeof(makecert_argv) / sizeof(char*));

int freerds_generate_certificate(rdpSettings* settings)
{
	char* freerds_home;
	char* server_file_path;
	MAKECERT_CONTEXT* context;

	freerds_home = freerds_get_home_path();

	server_file_path = GetCombinedPath(freerds_home, "etc/freerds");

	if (!PathFileExistsA(server_file_path))
		CreateDirectoryA(server_file_path, 0);

	settings->CertificateFile = GetCombinedPath(server_file_path, "server.crt");
	settings->PrivateKeyFile = GetCombinedPath(server_file_path, "server.key");
	settings->RdpKeyFile = GetCombinedPath(server_file_path, "server.key");

	if ((!PathFileExistsA(settings->CertificateFile)) ||
			(!PathFileExistsA(settings->PrivateKeyFile)))
	{
		context = makecert_context_new();

		makecert_context_process(context, makecert_argc, (char**) makecert_argv);

		makecert_context_set_output_file_name(context, "server");

		if (!PathFileExistsA(settings->CertificateFile))
			makecert_context_output_certificate_file(context, server_file_path);

		if (!PathFileExistsA(settings->PrivateKeyFile))
			makecert_context_output_private_key_file(context, server_file_path);

		makecert_context_free(context);
	}

	free(server_file_path);

	return 0;
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
	rdpSettings* settings = client->settings;

	settings->ColorDepth = 32;
	settings->NSCodec = TRUE;
	settings->RemoteFxCodec = TRUE;
	settings->BitmapCacheV3Enabled = TRUE;
	settings->FrameMarkerCommandEnabled = TRUE;
	settings->SurfaceFrameMarkerEnabled = TRUE;
	settings->SupportGraphicsPipeline = FALSE;

	settings->DrawAllowSkipAlpha = TRUE;
	settings->DrawAllowColorSubsampling = TRUE;
	settings->DrawAllowDynamicColorFidelity = TRUE;

	freerds_generate_certificate(settings);

	context->server = g_Server;
	context->channelServer = context->server->channels;

	context->id = context->server->connectionId++;

	context->settings = settings;
	context->bytesPerPixel = 4;

	context->FrameList = ListDictionary_New(TRUE);

	context->client = client;
	context->vcm = WTSOpenServerA((LPSTR) client->context);

	context->TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	context->notifications = MessageQueue_New(NULL);

	context->channels = ArrayList_New(TRUE);
	client->VirtualChannelRead = freerdp_client_virtual_channel_read;
}

void freerds_peer_context_free(freerdp_peer* client, rdsConnection* context)
{
	freerds_encoder_free(context->encoder);

	ListDictionary_Free(context->FrameList);

	WTSCloseServer((HANDLE) context->vcm);

	ArrayList_Free(context->channels);
}

BOOL freerds_peer_accepted(freerdp_listener* instance, freerdp_peer* client)
{
	rdsConnection* connection;

	client->ContextSize = sizeof(rdsConnection);
	client->ContextNew = (psPeerContextNew) freerds_peer_context_new;
	client->ContextFree = (psPeerContextFree) freerds_peer_context_free;
	freerdp_peer_context_new(client);

	connection = (rdsConnection*) client->context;

	connection->Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)
			freerds_connection_main_thread, client, 0, NULL);

	return TRUE;
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
	ChannelEvent = freerds_channel_server_listen_event(channels);

	while (1)
	{
		nCount = 0;
		events[nCount++] = TermEvent;
		events[nCount++] = ChannelEvent;

		nCount += listener->GetEventHandles(listener, events, 32 - nCount);

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
			freerds_channel_server_accept(channels);
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
