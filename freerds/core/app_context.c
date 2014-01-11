/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * Application Context
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bernhard.miklautz@thincast.com>
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

#include "app_context.h"
#include <winpr/interlocked.h>

static rdsAppContext* gAppContext = NULL;

void app_context_init()
{
	gAppContext = malloc(sizeof(rdsAppContext));
	gAppContext->connections = ListDictionary_New(TRUE);
	gAppContext->connectionId = 0;
}

void app_context_uninit()
{
	ListDictionary_Free(gAppContext->connections);	
	free(gAppContext);
	gAppContext = NULL;
}

long app_context_get_connectionid()
{
	return InterlockedIncrement(&(gAppContext->connectionId));
}

void app_context_add_connection(rdsConnection* connection)
{
	ListDictionary_Add(gAppContext->connections, (void*) connection->id, connection);
	printf("added connection %d\n", (int) connection->id);
}

void app_context_remove_connection(long id)
{
	ListDictionary_Remove(gAppContext->connections, (void*) id);
	printf("removed connection %d\n", (int) id);
}

rdsConnection* app_context_get_connection(long id)
{
	return ListDictionary_GetItemValue(gAppContext->connections, (void*) id);
}
