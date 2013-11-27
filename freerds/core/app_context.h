/**
 * FreeRDS
 * Application context
 *
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bmiklautz@thinstuff.at>
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

#ifndef FREERDS_APPCONTEXT_H
#define FREERDS_APPCONTEXT_H
#include <winpr/collections.h>
#include "freerds.h"

struct application_context 
{
	wListDictionary *connections;
	long connectionId;
};

typedef struct application_context rdsAppContext;

void app_context_init();
void app_context_uninit();
long app_context_get_connectionid();
void app_context_add_connection(rdsConnection *connection);
void app_context_remove_connection(long id);
#endif //FREERDS_APPCONTEXT_H
