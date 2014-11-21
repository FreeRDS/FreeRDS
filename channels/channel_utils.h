/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server utilities
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

#ifndef CHANNEL_UTILS_H
#define CHANNEL_UTILS_H

#include <winpr/wtypes.h>
#include <winpr/wtsapi.h>

#ifdef __cplusplus
extern "C" {
#endif

ULONG channel_utils_get_session_id();
WTS_CONNECTSTATE_CLASS channel_utils_get_session_state();

ULONG channel_utils_get_x11_display_num();

#ifdef __cplusplus
}
#endif

#endif
