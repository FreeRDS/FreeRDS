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

#ifndef FREERDS_MODULE_RDP_SERVICE_CLIENT_H
#define FREERDS_MODULE_RDP_SERVICE_CLIENT_H

#include "rdp_service.h"

#include <winpr/crt.h>
#include <winpr/wlog.h>
#include <winpr/synch.h>
#include <winpr/thread.h>

#include <freerdp/api.h>
#include <freerdp/freerdp.h>

#include <freerds/backend.h>

typedef struct rds_context rdsContext;

struct rds_context
{
	rdpContext context;
	DEFINE_RDP_CLIENT_COMMON();

	freerdp* instance;
	rdpSettings* settings;

	DWORD SessionId;
	rdsBackendService* service;

	wLog* log;

	HANDLE StopEvent;
	HANDLE UpdateThread;
	HANDLE ChannelsThread;

	int framebufferSize;
	RDS_FRAMEBUFFER framebuffer;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
* Client Interface
*/

FREERDP_API int RDS_RdpClientEntry(RDP_CLIENT_ENTRY_POINTS* pEntryPoints);

#ifdef __cplusplus
}
#endif

#endif /* FREERDS_MODULE_RDP_SERVICE_CLIENT_H */
