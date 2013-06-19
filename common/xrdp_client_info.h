/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
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
 *
 * xrdp / xserver info / caps
 */

#if !defined(XRDP_CLIENT_INFO_H)
#define XRDP_CLIENT_INFO_H

#include <freerdp/freerdp.h>

typedef struct xrdp_client_info xrdpClientInfo;

struct xrdp_client_info
{
	int size; /* bytes for this structure */
	int ColorDepth;
	int DesktopWidth;
	int DesktopHeight;
	/* bitmap cache info */
	int cache1_entries;
	int cache1_size;
	int cache2_entries;
	int cache2_size;
	int cache3_entries;
	int cache3_size;
	int BitmapCachePersistEnabled; /* 0 or 2 */
	int BitmapCacheVersion; /* ored 1 = original version, 2 = v2, 4 = v3 */
	/* pointer info */
	int PointerCacheSize;
	/* other */
	int BitmapCompressionDisabled;
	int BitmapCacheEnabled;
	char hostname[32];
	int ClientBuild;
	int KeyboardLayout;
	char username[256];
	char password[256];
	char domain[256];
	char program[256];
	char directory[256];
	int CompressionEnabled;
	int AutoLogonEnabled;
	int BrushSupportLevel;
	char client_ip[256];
	int OffscreenSupportLevel;
	int OffscreenCacheSize;
	int OffscreenCacheEntries;
	int rail_support_level;
	char orders[32];
	int order_flags_ex;
	int pointer_flags; /* 0 color, 1 new */
};

#endif
