/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * authentication library
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

#include <freerds/auth.h>

#include <winpr/crt.h>
#include <winpr/library.h>

pRdsAuthModuleEntry freerds_load_auth_module(const char* name)
{
        char* lowerName;
        HINSTANCE library;
        char moduleFileName[256];
        pRdsAuthModuleEntry moduleEntry;

        lowerName = _strdup(name);
        CharLowerA(lowerName);

        sprintf_s(moduleFileName, sizeof(moduleFileName), FREERDS_LIB_PATH "/libfreerds-auth-%s.so", lowerName);

        free(lowerName);

        library = LoadLibraryA(moduleFileName);

        if (!library)
                return NULL;

        moduleEntry = GetProcAddress(library, RDS_AUTH_MODULE_ENTRY_POINT_NAME);

        if (moduleEntry)
                return moduleEntry;

        FreeLibrary(library);

        return NULL;
}
