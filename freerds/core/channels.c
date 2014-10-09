/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013-2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include <winpr/crt.h>
#include <winpr/rpc.h>
#include <winpr/file.h>
#include <winpr/path.h>

#ifndef _WIN32
#include <netinet/tcp.h>
#endif

#include "channels.h"

//#define WITH_FREERDS_CHANNELS	1

#include "rpc.h"

BOOL freerds_channels_is_channel_allowed(UINT32 SessionId, char* ChannelName)
{
	FDSAPI_CHANNEL_ALLOWED_REQUEST request;
	FDSAPI_CHANNEL_ALLOWED_RESPONSE response;

	response.ChannelAllowed = FALSE;

	freerds_icp_IsChannelAllowed(&request, &response);

	return response.ChannelAllowed;
}

int freerds_channels_post_connect(rdsConnection* session)
{
	BOOL allowed;

	if (WTSVirtualChannelManagerIsChannelJoined(session->vcm, "cliprdr"))
	{
		allowed = freerds_channels_is_channel_allowed(session->id, "cliprdr");
		printf("channel %s is %s\n", "cliprdr", allowed ? "allowed" : "not allowed");

#ifdef WITH_FREERDS_CHANNELS
		printf("Channel %s registered\n", "cliprdr");
		session->cliprdr = cliprdr_server_context_new(session->vcm);
		session->cliprdr->Start(session->cliprdr);
#endif
	}

	if (WTSVirtualChannelManagerIsChannelJoined(session->vcm, "rdpdr"))
	{
		allowed = freerds_channels_is_channel_allowed(session->id, "rdpdr");
		printf("channel %s is %s\n", "rdpdr", allowed ? "allowed" : "not allowed");

#ifdef WITH_FREERDS_CHANNELS
		printf("Channel %s registered\n", "rdpdr");
		session->rdpdr = rdpdr_server_context_new(session->vcm);
		session->rdpdr->Start(session->rdpdr);
#endif
	}

	if (WTSVirtualChannelManagerIsChannelJoined(session->vcm, "rdpsnd"))
	{
		allowed = freerds_channels_is_channel_allowed(session->id, "rdpsnd");
		printf("channel %s is %s\n", "rdpsnd", allowed ? "allowed" : "not allowed");

#ifdef WITH_FREERDS_CHANNELS
		printf("Channel %s registered\n", "rdpsnd");
		session->rdpsnd = rdpsnd_server_context_new(session->vcm);
		session->rdpsnd->Start(session->rdpsnd);
#endif
	}

	return 0;
}

/* channel */

rdsChannel* freerds_channel_new(rdsConnection* connection, const char* name)
{
	rdsChannel* channel;

	channel = (rdsChannel*) calloc(1, sizeof(rdsChannel));

	if (channel)
	{
		RPC_CSTR rpcString = NULL;

		channel->name = _strdup(name);

		UuidCreateSequential(&(channel->guid));

		UuidToStringA(&(channel->guid), &rpcString);
		channel->guidString = _strdup(rpcString);
		RpcStringFreeA(&rpcString);
	}

	return channel;
}

void freerds_channel_free(rdsChannel* channel)
{
	if (!channel)
		return;

	free(channel->name);
	free(channel->guidString);

	free(channel);
}

/* channel server */

int freerds_channels_open(rdsChannels* channels)
{
	int status = 0;
	char servname[64] = { 0 };
	struct addrinfo hints = { 0 };
	struct addrinfo* result = NULL;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	sprintf_s(servname, sizeof(servname), "%d", channels->listenPort);
	getaddrinfo(channels->listenAddress, servname, &hints, &result);

	if (status != 0)
	{
		fprintf(stderr, "getaddrinfo failure: %d\n", status);
		return -1;
	}

	channels->listenerSocket = _socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (channels->listenerSocket == INVALID_SOCKET)
	{
		fprintf(stderr, "socket failure: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		return -1;
	}

	status = _bind(channels->listenerSocket, result->ai_addr, (int) result->ai_addrlen);

	if (status == SOCKET_ERROR)
	{
		fprintf(stderr, "bind failure: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(channels->listenerSocket);
		return -1;
	}

	freeaddrinfo(result);

	status = _listen(channels->listenerSocket, SOMAXCONN);

	if (status == SOCKET_ERROR)
	{
		fprintf(stderr, "listen failure: %d\n", WSAGetLastError());
		closesocket(channels->listenerSocket);
		return -1;
	}

	channels->listenEvent = CreateFileDescriptorEvent(NULL, FALSE, FALSE, (int) channels->listenerSocket);

	fprintf(stderr, "Listening on %s:%d for channels...\n", channels->listenAddress, channels->listenPort);

	return 1;
}

int freerds_channels_close(rdsChannels* channels)
{
	if (channels->listenerSocket)
	{
		closesocket(channels->listenerSocket);
		channels->listenerSocket = 0;
	}

	if (channels->listenEvent)
	{
		CloseHandle(channels->listenEvent);
		channels->listenEvent = NULL;
	}

	return 1;
}

HANDLE freerds_channels_get_event_handle(rdsChannels* channels)
{
	return channels->listenEvent;
}

int freerds_channels_check_socket(rdsChannels* channels)
{
	int status;
	int optlen = 0;
	UINT32 optval = 0;
	SOCKET clientSocket;
	char guidString[36 + 1];

	clientSocket = accept(channels->listenerSocket, NULL, NULL);

	if (clientSocket == INVALID_SOCKET)
	{
		fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
		return -1;
	}

	fprintf(stderr, "channel connection accepted!\n");

	/* receive channel connection GUID */

	guidString[36] = '\0';
	status = _recv(clientSocket, guidString, 36, 0);

	if (status != 36)
	{
		return -1;
	}

	fprintf(stderr, "channel GUID: %s\n", guidString);

	optval = TRUE;
	optlen = sizeof(optval);

	_setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*) &optval, optlen);

	return 1;
}

rdsChannels* freerds_channels_new()
{
	rdsChannels* channels;

	channels = (rdsChannels*) calloc(1, sizeof(rdsChannels));

	if (channels)
	{
		channels->listenPort = 40123;
		channels->listenAddress = _strdup("127.0.0.1");
	}

	return channels;
}

void freerds_channels_free(rdsChannels* channels)
{
	if (!channels)
		return;

	free(channels);
}

