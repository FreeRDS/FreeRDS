/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server plugin interface
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

#ifndef CHANNEL_PLUGIN_H
#define CHANNEL_PLUGIN_H

#include <winpr/wtypes.h>

typedef struct vcplugin_t VCPlugin;

struct vcplugin_t
{
	HMODULE hModule;

	ULONG sessionId;
	ULONG x11DisplayNum;

	const char *name;
	void *context;

	BOOL (*OnPluginInitialize)(VCPlugin *);
	void (*OnPluginTerminate)(VCPlugin *);

	void (*OnSessionCreate)(VCPlugin *);
	void (*OnSessionDelete)(VCPlugin *);
	void (*OnSessionConnect)(VCPlugin *);
	void (*OnSessionDisconnect)(VCPlugin *);
	void (*OnSessionLogon)(VCPlugin *);
	void (*OnSessionLogoff)(VCPlugin *);
};

typedef BOOL (*LPVCPluginEntryPoint)(VCPlugin *);

#endif
