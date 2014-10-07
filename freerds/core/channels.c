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
