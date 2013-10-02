/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * RDP Module Service
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

#include <freerdp/utils/event.h>
#include <freerdp/client/channels.h>

#include "rdp_client.h"

#include "rdp_channels.h"

void rds_OnChannelConnectedEventHandler(rdpContext* context, ChannelConnectedEventArgs* e)
{
	rdsContext* rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "OnChannelConnected: %s", e->name);
}

void rds_OnChannelDisconnectedEventHandler(rdpContext* context, ChannelDisconnectedEventArgs* e)
{
	rdsContext* rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "OnChannelDisconnected: %s", e->name);
}

void rds_process_channel_event(rdpChannels* channels, freerdp* instance)
{
	rdsContext* rds;
	wMessage* event;

	rds = (rdsContext*) instance->context;

	event = freerdp_channels_pop_event(channels);

	if (event)
	{
		switch (GetMessageClass(event->id))
		{
			case RailChannel_Class:
				break;

			case TsmfChannel_Class:
				break;

			case CliprdrChannel_Class:
				break;

			case RdpeiChannel_Class:
				break;

			default:
				break;
		}

		freerdp_event_free(event);
	}
}

void* rds_channels_thread(void* arg)
{
	int status;
	DWORD nCount;
	rdsContext* rds;
	HANDLE events[8];
	rdpChannels* channels;
	freerdp* instance = (freerdp*) arg;

	rds = (rdsContext*) instance->context;
	channels = instance->context->channels;

	nCount = 0;
	events[nCount++] = rds->StopEvent;
	events[nCount++] = freerdp_channels_get_event_handle(instance);

	while (1)
	{
		WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(rds->StopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		status = freerdp_channels_process_pending_messages(instance);

		if (!status)
			break;

		rds_process_channel_event(channels, instance);
	}

	ExitThread(0);
	return NULL;
}

int rds_receive_channel_data(freerdp* instance, int channelId, BYTE* data, int size, int flags, int totalSize)
{
	return freerdp_channels_data(instance, channelId, data, size, flags, totalSize);
}
