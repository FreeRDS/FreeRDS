/**
 * FreeRDS internal communication protocol
 * Server stubs - can be called from session-manager over rpc 
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

#include "icp_server_stubs.h"

#include "ICP.pb-c.h"
#include "pbrpc.h"
#include "../core/app_context.h"

#define ICP_SERVER_STUB_SETUP(camel, expanded) \
	Freerds__Icp__##camel ##Request *request; \
	Freerds__Icp__##camel ##Response response; \
	pbRPCPayload *payload; \
	int ret = 0; \
	freerds__icp__##expanded ##_response__init(&response); \
	request = freerds__icp__##expanded ##_request__unpack(NULL, pbrequest->dataLen, (uint8_t*)pbrequest->data);\
	if (!request) \
	{ \
		return PBRPC_BAD_REQEST_DATA; \
	}

#define ICP_SERVER_STUB_RESPOND(camel, expanded) \
	freerds__icp__##expanded ##_request__free_unpacked(request, NULL); \
	payload = pbrpc_payload_new(); \
	payload->dataLen = freerds__icp__##expanded ##_response__get_packed_size(&response); \
	payload->data = malloc(payload->dataLen); \
	ret = freerds__icp__##expanded ##_response__pack(&response, (uint8_t*) payload->data); \
	if (ret != payload->dataLen) \
	{ \
		free(payload->data); \
		return PBRPC_BAD_RESPONSE; \
	} \
	*pbresponse = payload;

int ping(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	ICP_SERVER_STUB_SETUP(Ping, ping)

	// call functions with parameters from request and set answer to response
	response.pong = TRUE;

	ICP_SERVER_STUB_RESPOND(Ping, ping)

	// freeup response data if necessary

	return PBRPC_SUCCESS;
}

int switchTo(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	rdsConnection *connection = NULL;
	ICP_SERVER_STUB_SETUP(SwitchTo, switch_to)
	connection = app_context_get_connection(request->connectionid);
	if (connection)
	{
		struct rds_notification_msg_switch *msg = malloc(sizeof(struct rds_notification_msg_switch));
		msg->tag = tag;
		msg->endpoint = _strdup(request->serviceendpoint);
		MessageQueue_Post(connection->notifications, (void *)connection, NOTIFY_SWITCHTO, (void*) msg, NULL);
		freerds__icp__switch_to_request__free_unpacked(request, NULL);
		// response is sent after processing the notification
		*pbresponse = NULL;
		return 0;
	}
	else
	{
		fprintf(stderr, "something went wrong\n");
		response.success = FALSE;
	}

	ICP_SERVER_STUB_RESPOND(SwitchTo, switch_to)
	return PBRPC_SUCCESS;
}

int logOffUserSession(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	rdsConnection *connection = NULL;
	ICP_SERVER_STUB_SETUP(LogOffUserSession, log_off_user_session)
	connection = app_context_get_connection(request->connectionid);
	if (connection)
	{
		struct rds_notification_msg_logoff *msg = malloc(sizeof(struct rds_notification_msg_logoff));
		msg->tag = tag;
		MessageQueue_Post(connection->notifications, (void *)connection, NOTIFY_LOGOFF, (void*) msg, NULL);
		freerds__icp__log_off_user_session_request__free_unpacked(request, NULL);
		// response is sent after processing the notification
		*pbresponse = NULL;
		return 0;
	}
	else
	{
		fprintf(stderr, "something went wrong\n");
		response.loggedoff = FALSE;
	}

	ICP_SERVER_STUB_RESPOND(LogOffUserSession, log_off_user_session)
	return PBRPC_SUCCESS;
}
