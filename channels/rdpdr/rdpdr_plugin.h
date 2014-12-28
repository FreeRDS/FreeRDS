/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server plugin for RDPDR
 *
 * Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
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

#ifndef _RDPDR_PLUGIN_H
#define _RDPDR_PLUGIN_H

#include <winpr/wlog.h>
#include <winpr/wtypes.h>

#include <freerdp/server/rdpdr.h>

typedef struct
{
	wLog *log;

	HANDLE hThread;
	DWORD dwThreadId;

	/* Used to implement the RDPDR protocol. */
	RdpdrServerContext *rdpdrServer;
} RdpdrPluginContext;

#endif
