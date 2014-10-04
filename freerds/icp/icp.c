/**
 * FreeRDS internal communication protocol
 * Main functionality
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

#include <freerds/icp.h>

#include "pbrpc.h"
#include "icp_server_stubs.h"
#include "ICP.pb-c.h"

static pbRPCContext* g_pbContext = NULL;

int freerds_icp_start()
{
	if (!g_pbContext)
	{
		g_pbContext = pbrpc_server_new();
		pbrpc_server_start(g_pbContext);
	}

	return 0;
}

int freerds_icp_shutdown()
{
	if (g_pbContext)
	{
		pbrpc_server_stop(g_pbContext);
		pbrpc_server_free(g_pbContext);
		g_pbContext = NULL;
	}

	return 0;
}

void* freerds_icp_get_context()
{
	return (void*) g_pbContext;
}
