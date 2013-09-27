/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * RDP Server Module
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

#include <winpr/crt.h>

#include <freerds/freerds.h>

#include "rdp_module.h"

struct rds_module_rdp
{
	rdsModule module;
};
typedef struct rds_module_rdp rdsModuleRdp;

int rdp_rds_module_new(rdsModule* module)
{
	return 0;
}

void rdp_rds_module_free(rdsModule* module)
{

}

int rdp_rds_module_start(rdsModule* module)
{
	return 0;
}

int rdp_rds_module_stop(rdsModule* module)
{
	SetEvent(module->StopEvent);

	return 0;
}

int RDP_RdsModuleEntry(RDS_MODULE_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Size = sizeof(RDS_MODULE_ENTRY_POINTS_V1);

	pEntryPoints->ContextSize = sizeof(rdsModuleRdp);
	pEntryPoints->New = rdp_rds_module_new;
	pEntryPoints->Free = rdp_rds_module_free;

	pEntryPoints->Start = rdp_rds_module_start;
	pEntryPoints->Stop = rdp_rds_module_stop;

	return 0;
}

