/**
 * pbRPC - a simple, protocol buffer based RCP mechanism
 * Utility functions
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

#include <winpr/crt.h>
#include <winpr/interlocked.h>

#include "pbRPC.pb-c.h"
#include "pbrpc_utils.h"

DWORD pbrpc_getTag(pbRPCContext *context)
{
	return InterlockedIncrement(&(context->tag));
}

Freerds__Pbrpc__RPCBase* pbrpc_message_new()
{
	Freerds__Pbrpc__RPCBase* msg = malloc(sizeof(Freerds__Pbrpc__RPCBase));
	if (!msg)
		return msg;

	freerds__pbrpc__rpcbase__init(msg);
	return msg;
}

void pbrpc_message_free(Freerds__Pbrpc__RPCBase* msg, BOOL freePayload)
{
	if (freePayload && msg->payload.data)
	{
		free(msg->payload.data);
	}

	if (freePayload && msg->errordescription)
	{
		free(msg->errordescription);
	}

	free(msg);
}

void pbrpc_prepare_request(pbRPCContext* context, Freerds__Pbrpc__RPCBase* msg)
{
	msg->tag = pbrpc_getTag(context);
	msg->isresponse = FALSE;
	msg->status = FREERDS__PBRPC__RPCBASE__RPCSTATUS__SUCCESS;
}

void pbrpc_prepare_response(Freerds__Pbrpc__RPCBase* msg, UINT32 tag)
{
	msg->isresponse = TRUE;
	msg->tag = tag;
}

void pbrpc_prepare_error(Freerds__Pbrpc__RPCBase* msg, UINT32 tag, char *error)
{
	pbrpc_prepare_response(msg, tag);
	msg->status = FREERDS__PBRPC__RPCBASE__RPCSTATUS__FAILED;
	msg->errordescription = error;
}

pbRPCPayload* pbrpc_payload_new()
{
	pbRPCPayload* pl = malloc(sizeof(pbRPCPayload));
	ZeroMemory(pl, sizeof(pbRPCPayload));
	return pl;
}

void pbrpc_free_payload(pbRPCPayload* response)
{
	if (!response)
		return;

	free(response->data);

	if (response->errorDescription)
		free(response->errorDescription);

	free(response);
}
