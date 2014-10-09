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
	BOOL connected;
	SOCKET socket;
	rdsServer* server;
	rdsConnection* connection;
	rdsChannelServer* channels;
};

int freerds_channels_post_connect(rdsConnection* session);

rdsChannel* freerds_channel_new(rdsConnection* connection, const char* name);
void freerds_channel_free(rdsChannel* channel);

int freerds_channel_server_open(rdsChannelServer* channels);
int freerds_channel_server_close(rdsChannelServer* channels);

HANDLE freerds_channel_server_listen_event(rdsChannelServer* channels);
int freerds_channel_server_accept(rdsChannelServer* channels);

rdsChannelServer* freerds_channel_server_new();
void freerds_channel_server_free(rdsChannelServer* channels);

#endif /* FREERDS_CHANNELS_H */
