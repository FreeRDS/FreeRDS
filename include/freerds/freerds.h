/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
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

#ifndef FREERDS_H
#define FREERDS_H

#include <xrdp-ng/xrdp.h>

/**
 * Module Entry Points
 */

typedef int (*pRdsModuleNew)(rdsModule* module);
typedef void (*pRdsModuleFree)(rdsModule* module);

typedef int (*pRdsModuleStart)(rdsModule* module);
typedef int (*pRdsModuleStop)(rdsModule* module);

struct rds_module_entry_points_v1
{
	DWORD Size;
	DWORD Version;

	DWORD ContextSize;
	pRdsModuleNew New;
	pRdsModuleFree Free;

	pRdsModuleStart Start;
	pRdsModuleStop Stop;
};

#define RDS_MODULE_INTERFACE_VERSION	1
#define RDS_MODULE_ENTRY_POINT_NAME	"RdsModuleEntry"

typedef int (*pRdsModuleEntry)(RDS_MODULE_ENTRY_POINTS* pEntryPoints);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* FREERDS_H */
