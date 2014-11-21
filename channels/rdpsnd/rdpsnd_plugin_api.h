/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server plugin API for RDPSND
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

#ifndef RDPSND_PLUGIN_API_H
#define RDPSND_PLUGIN_API_H

#include <winpr/wtypes.h>

void *rdpsnd_plugin_api_new();
void rdpsnd_plugin_api_free(void *api_context);

BOOL rdpsnd_plugin_api_connect(void *api_context);
BOOL rdpsnd_plugin_api_disconnect(void *api_context);

BOOL rdpsnd_plugin_api_play_audio(void *api_context, BYTE *buffer, UINT32 length);

#endif
