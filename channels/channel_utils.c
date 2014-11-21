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

#include <winpr/crt.h>

#include "channel_utils.h"

ULONG channel_utils_get_session_id()
{
	ULONG ulSessionId;
	BOOL bSuccess;
	LPSTR pszBuffer;
	DWORD cbBuffer;

	ulSessionId = 0;

	bSuccess = WTSQuerySessionInformation(
		WTS_CURRENT_SERVER_HANDLE,
		WTS_CURRENT_SESSION,
		WTSSessionId,
		&pszBuffer,
		&cbBuffer);

	if (bSuccess && (cbBuffer == sizeof(ULONG)))
	{
		ulSessionId = *(ULONG *)pszBuffer;

		WTSFreeMemory(pszBuffer);
	}

	return ulSessionId;
}

WTS_CONNECTSTATE_CLASS channel_utils_get_session_state()
{
	WTS_CONNECTSTATE_CLASS sessionState;
	BOOL bSuccess;
	LPSTR pszBuffer;
	DWORD cbBuffer;

	sessionState = WTSDown;

	bSuccess = WTSQuerySessionInformation(
		WTS_CURRENT_SERVER_HANDLE,
		WTS_CURRENT_SESSION,
		WTSConnectState,
		&pszBuffer,
		&cbBuffer);

	if (bSuccess && (cbBuffer == sizeof(WTS_CONNECTSTATE_CLASS)))
	{
		sessionState = *(WTS_CONNECTSTATE_CLASS *)pszBuffer;

		WTSFreeMemory(pszBuffer);
	}

	return sessionState;
}

ULONG channel_utils_get_x11_display_num()
{
    int index;
    int mode;
    int host_index;
    int disp_index;
    int scre_index;
	char *display_text;
    int display_num;
    char host[256];
    char disp[256];
    char scre[256];

	display_text = getenv("DISPLAY");
    if (display_text == NULL) {
        return 0;
    }
    memset(host, 0, 256);
    memset(disp, 0, 256);
    memset(scre, 0, 256);

    index = 0;
    host_index = 0;
    disp_index = 0;
    scre_index = 0;
    mode = 0;

    while (display_text[index] != 0) {
        if (display_text[index] == ':') {
            mode = 1;
        } else if (display_text[index] == '.') {
            mode = 2;
        } else if (mode == 0) {
            host[host_index] = display_text[index];
            host_index++;
        } else if (mode == 1) {
            disp[disp_index] = display_text[index];
            disp_index++;
        } else if (mode == 2) {
            scre[scre_index] = display_text[index];
            scre_index++;
        }
        index++;
    }

    host[host_index] = 0;
    disp[disp_index] = 0;
    scre[scre_index] = 0;
    display_num = atoi(disp);
    return display_num;
}
