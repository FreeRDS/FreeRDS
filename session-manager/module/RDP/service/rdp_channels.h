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

#ifndef FREERDS_MODULE_RDP_SERVICE_CHANNELS_H
#define FREERDS_MODULE_RDP_SERVICE_CHANNELS_H

#include "rdp_service.h"

void* rds_channels_thread(void* arg);

void rds_OnChannelConnectedEventHandler(rdpContext* context, ChannelConnectedEventArgs* e);
void rds_OnChannelDisconnectedEventHandler(rdpContext* context, ChannelDisconnectedEventArgs* e);

int rds_receive_channel_data(freerdp* instance, int channelId, BYTE* data, int size, int flags, int totalSize);

#endif /* FREERDS_MODULE_RDP_SERVICE_CHANNELS_H */
