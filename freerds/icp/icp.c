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
#include "pipe_transport.h"
#include "icp_server_stubs.h"
#include "ICP.pb-c.h"

struct icp_context
{
	pbRPCContext* pbcontext;
	pbRPCTransportContext* tpcontext;
};

static struct icp_context* icpContext = NULL;

static pbRPCMethod icpMethods[] =
{
	{ FREERDS__ICP__MSGTYPE__Ping, ping },
	{ FREERDS__ICP__MSGTYPE__SwitchTo, switchTo},
	{ FREERDS__ICP__MSGTYPE__LogOffUserSession, logOffUserSession},
	{ 0, NULL }
};

int freerds_icp_start()
{
	icpContext = malloc(sizeof(struct icp_context));
	icpContext->tpcontext = tp_npipe_new();
	icpContext->pbcontext = pbrpc_server_new(icpContext->tpcontext);

	pbrpc_register_methods(icpContext->pbcontext, icpMethods);
	pbrpc_server_start(icpContext->pbcontext);
	return 0;
}

int freerds_icp_shutdown()
{
	pbrpc_server_stop(icpContext->pbcontext);
	pbrpc_server_free(icpContext->pbcontext);
	tp_npipe_free(icpContext->tpcontext);
	free(icpContext);
	return 0;
}

void* freerds_icp_get_context()
{
	return icpContext->pbcontext;
}
