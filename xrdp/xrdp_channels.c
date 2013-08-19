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

int xrdp_channels_post_connect(xrdpSession* session)
{
	int i;
	rdpSettings* settings = session->settings;

	for (i = 0; i < settings->ChannelCount; i++)
	{
		if (settings->ChannelDefArray[i].joined)
		{
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
			else
			{
				printf("Channel %s not registered\n", settings->ChannelDefArray[i].Name);
			}
		}
	}

	return 0;
}
