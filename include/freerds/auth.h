/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * Authentication Module Interface
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

#ifndef FREERDS_AUTH_H
#define FREERDS_AUTH_H

#include <freerds/api.h>

#include <winpr/wtypes.h>

typedef struct rds_auth_module_entry_points_v1 RDS_AUTH_MODULE_ENTRY_POINTS_V1;
typedef RDS_AUTH_MODULE_ENTRY_POINTS_V1 RDS_AUTH_MODULE_ENTRY_POINTS;

/**
 * Authentication Module Entry Points
 */

#ifdef __cplusplus
extern "C" {
#endif

struct _rds_auth_module
{
	void* dummy;
};
typedef struct _rds_auth_module rdsAuthModule;

typedef rdsAuthModule* (*pRdsAuthModuleNew)(void);
typedef void (*pRdsAuthModuleFree)(rdsAuthModule* auth);

typedef char* (*pRdsAuthModuleStart)(rdsAuthModule* auth);
typedef int (*pRdsAuthModuleStop)(rdsAuthModule* auth);

typedef int (*pRdsAuthLogonUser)(rdsAuthModule* auth, const char* username, const char* domain, const char* password);

struct rds_auth_module_entry_points_v1
{
	DWORD Version;

	pRdsAuthModuleNew New;
	pRdsAuthModuleFree Free;

	pRdsAuthLogonUser LogonUser;
};

#define RDS_AUTH_MODULE_INTERFACE_VERSION	1
#define RDS_AUTH_MODULE_ENTRY_POINT_NAME	"RdsAuthModuleEntry"

typedef int (*pRdsAuthModuleEntry)(RDS_AUTH_MODULE_ENTRY_POINTS* pEntryPoints);

FREERDS_EXPORT pRdsAuthModuleEntry freerds_load_auth_module(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* FREERDS_AUTH_H */
