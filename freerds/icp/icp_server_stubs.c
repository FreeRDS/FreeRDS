/**
 * FreeRDS internal communication protocol
 * Server stubs - can be called from session-manager over rpc 
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
#include "icp_server_stubs.h"
#include "ICP.pb-c.h"
#include "pbrpc_utils.h"

#define ICP_SERVER_STUB_SETUP(camel, expanded) \
	FreeRDS__Icp__##camel ##Request *request; \
	FreeRDS__Icp__##camel ##Response response; \
	pbRPCPayload *payload; \
	int ret = 0; \
	free_rds__icp__##expanded ##_response__init(&response); \
	request = free_rds__icp__##expanded ##_request__unpack(NULL, pbrequest->dataLen, (uint8_t*)pbrequest->data);\
	if (!request) \
	{ \
		return PBRPC_BAD_REQEST_DATA; \
	}

#define ICP_SERVER_STUB_RESPOND(camel, expanded) \
	free_rds__icp__##expanded ##_request__free_unpacked(request, NULL); \
	payload = pbrpc_payload_new(); \
	payload->dataLen = free_rds__icp__##expanded ##_response__get_packed_size(&response); \
	payload->data = malloc(payload->dataLen); \
	ret = free_rds__icp__##expanded ##_response__pack(&response, (uint8_t*) payload->data); \
	if (ret != payload->dataLen) \
	{ \
		free(payload->data); \
		return PBRPC_BAD_RESPONSE; \
	} \
	*pbresponse = payload;

int ping(pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	ICP_SERVER_STUB_SETUP(Ping, ping)

	// call functions with parameters from request and set answer to response
	response.pong = TRUE;

	ICP_SERVER_STUB_RESPOND(Ping, ping)

	// freeup response data if necessary

	return PBRPC_SUCCESS;
}
