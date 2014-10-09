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

struct rds_channels
{
	int listenPort;
	char* listenAddress;
	SOCKET listenerSocket;
	HANDLE listenEvent;
};

struct rds_channel
{
	char* name;
	GUID guid;
	char* guidString;
	rdsConnection* connection;
};

int freerds_channels_post_connect(rdsConnection* session);

rdsChannel* freerds_channel_new(rdsConnection* connection, const char* name);
void freerds_channel_free(rdsChannel* channel);

int freerds_channels_open(rdsChannels* channels);
int freerds_channels_close(rdsChannels* channels);

HANDLE freerds_channels_get_event_handle(rdsChannels* channels);
int freerds_channels_check_socket(rdsChannels* channels);

rdsChannels* freerds_channels_new();
void freerds_channels_free(rdsChannels* channels);

#endif /* FREERDS_CHANNELS_H */
