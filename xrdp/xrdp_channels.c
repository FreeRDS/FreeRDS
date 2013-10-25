 /**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng channels
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

#include "xrdp_channels.h"
#include <freerds/icp_client_stubs.h>

int xrdp_channels_post_connect(rdsConnection* session)
{
	int i;
	rdpSettings* settings = session->settings;

	for (i = 0; i < settings->ChannelCount; i++)
	{

		if (settings->ChannelDefArray[i].joined)
		{
			BOOL allowed = FALSE;

			freerds_icp_IsChannelAllowed(session->id, settings->ChannelDefArray[i].Name, &allowed);
			printf("channel %s is %s\n", settings->ChannelDefArray[i].Name, allowed ? "allowed" : "not allowed");
#if 0
			if (strncmp(settings->ChannelDefArray[i].Name, "cliprdr", 7) == 0)
			{
				printf("Channel %s registered\n", settings->ChannelDefArray[i].Name);
				session->cliprdr = cliprdr_server_context_new(session->vcm);
				session->cliprdr->Start(session->cliprdr);
			}
			else if (strncmp(settings->ChannelDefArray[i].Name, "rdpdr", 5) == 0)
			{
				printf("Channel %s registered\n", settings->ChannelDefArray[i].Name);
				session->rdpdr = rdpdr_server_context_new(session->vcm);
				session->rdpdr->Start(session->rdpdr);
			}
			else if (strncmp(settings->ChannelDefArray[i].Name, "rdpsnd", 6) == 0)
			{
				printf("Channel %s registered\n", settings->ChannelDefArray[i].Name);
				session->rdpsnd = rdpsnd_server_context_new(session->vcm);
				session->rdpsnd->Start(session->rdpsnd);
			}
			else if (strncmp(settings->ChannelDefArray[i].Name, "drdynvc", 7) == 0)
			{
				printf("Channel %s registered\n", settings->ChannelDefArray[i].Name);
				session->drdynvc = drdynvc_server_context_new(session->vcm);
				session->drdynvc->Start(session->drdynvc);
			}
			else
#endif
			{
				printf("Channel %s not registered\n", settings->ChannelDefArray[i].Name);
			}
		}
	}

	return 0;
}
