/**
 * FreeRDS service helper
 * Common functionality that can be used by services
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bmiklautz@thinstuff.at>
 *
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

#ifndef FREERDS_SERVICE_HELPER_H
#define FREERDS_SERVICE_HELPER_H

#include <freerds/freerds.h>
#include <freerds/module_connector.h>

/**
 * Service Interface
 */

typedef struct rds_service rdsService;

typedef int (*pRdsServiceAccept)(rdsService* service);

struct rds_service
{
       rdsModuleConnector connector;

       void* custom;

       HANDLE StopEvent;

       HANDLE ClientThread;
       HANDLE ServerThread;

       pRdsServiceAccept Accept;
};

#ifdef __cplusplus
extern "C" {
#endif

int freerds_service_start(rdsService* service);
int freerds_service_stop(rdsService* service);

rdsService* freerds_service_new(DWORD SessionId, const char* endpoint);
void freerds_service_free(rdsService* service);

#ifdef __cplusplus
}
#endif


#endif /* FREERDS_SERVICE_HELPER_H */
