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

#ifndef FREERDS_CHANNELS_H
#define FREERDS_CHANNELS_H

#include "freerds.h"

#include <winpr/winsock.h>
#include <winpr/collections.h>

struct rds_channel_server
{
	int listenPort;
	char* listenAddress;
	SOCKET listenerSocket;
	HANDLE listenEvent;
	wHashTable* table;
};

struct rds_channel
{
	char* name;
	UINT32 port;
	GUID guid;
	char* guidString;
	SOCKET socket;
	HANDLE event;
	BOOL connected;
	HANDLE readyEvent;
	HANDLE rdpChannel;
	rdsServer* server;
	rdsConnection* connection;
	rdsChannelServer* channels;
};

int freerds_channels_post_connect(rdsConnection* session);

int freerdp_client_virtual_channel_read(freerdp_peer* client, HANDLE hChannel, BYTE* buffer, UINT32 length);

int freerds_client_add_channel(rdsConnection* connection, rdsChannel* channel);
int freerds_client_remove_channel(rdsConnection* connection, rdsChannel* channel);

int freerds_client_get_channel_event_handles(rdsConnection* connection, HANDLE* events, DWORD* nCount);
int freerds_client_check_channel_event_handles(rdsConnection* connection);

rdsChannel* freerds_channel_new(rdsConnection* connection, const char* name);
void freerds_channel_free(rdsChannel* channel);

int freerds_channel_server_open(rdsChannelServer* channels);
int freerds_channel_server_close(rdsChannelServer* channels);

int freerds_channel_server_add(rdsChannelServer* channels, rdsChannel* channel);
int freerds_channel_server_remove(rdsChannelServer* channels, rdsChannel* channel);

HANDLE freerds_channel_server_listen_event(rdsChannelServer* channels);
int freerds_channel_server_accept(rdsChannelServer* channels);

rdsChannelServer* freerds_channel_server_new();
void freerds_channel_server_free(rdsChannelServer* channels);

#endif /* FREERDS_CHANNELS_H */
