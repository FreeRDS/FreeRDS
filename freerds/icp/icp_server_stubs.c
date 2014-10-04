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

int ping(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	int ret = 0;
	pbRPCPayload* payload;
	Freerds__Icp__PingRequest* request;
	Freerds__Icp__PingResponse response;

	freerds__icp__ping_response__init(&response);
	request = freerds__icp__ping_request__unpack(NULL, pbrequest->length, pbrequest->buffer);

	if (!request)
		return PBRPC_BAD_REQEST_DATA;

	response.pong = TRUE;

	freerds__icp__ping_request__free_unpacked(request, NULL);

	payload = pbrpc_payload_new();
	payload->length = freerds__icp__ping_response__get_packed_size(&response);
	payload->buffer = malloc(payload->length);

	ret = freerds__icp__ping_response__pack(&response, payload->buffer);

	if (ret != payload->length)
	{
		free(payload->buffer);
		return PBRPC_BAD_RESPONSE;
	}
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

int switchTo(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	int ret = 0;
	pbRPCPayload* payload;
	rdsConnection* connection = NULL;
	Freerds__Icp__SwitchToRequest* request;
	Freerds__Icp__SwitchToResponse response;

	freerds__icp__switch_to_response__init(&response);
	request = freerds__icp__switch_to_request__unpack(NULL, pbrequest->length, pbrequest->buffer);

	if (!request)
		return PBRPC_BAD_REQEST_DATA;

	connection = app_context_get_connection(request->connectionid);

	if (connection)
	{
		FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST* pRequest;

		pRequest = (FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST*) malloc(sizeof(FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST));

		pRequest->callId = tag;
		pRequest->msgType = FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID;
		pRequest->ServiceEndpoint = _strdup(request->serviceendpoint);
		freerds__icp__switch_to_request__free_unpacked(request, NULL);

		MessageQueue_Post(connection->notifications, (void*) connection, pRequest->msgType, (void*) pRequest, NULL);
		*pbresponse = NULL;
		return 0;
	}
	else
	{
		fprintf(stderr, "something went wrong\n");
		response.success = FALSE;
	}

	freerds__icp__switch_to_request__free_unpacked(request, NULL);

	payload = pbrpc_payload_new();
	payload->length = freerds__icp__switch_to_response__get_packed_size(&response);
	payload->buffer = malloc(payload->length);

	ret = freerds__icp__switch_to_response__pack(&response, payload->buffer);

	if (ret != payload->length)
	{
		free(payload->buffer);
		return PBRPC_BAD_RESPONSE;
	}
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

int logOffUserSession(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	int ret = 0;
	pbRPCPayload* payload;
	rdsConnection* connection = NULL;
	Freerds__Icp__LogOffUserSessionRequest* request;
	Freerds__Icp__LogOffUserSessionResponse response;

	freerds__icp__log_off_user_session_response__init(&response);
	request = freerds__icp__log_off_user_session_request__unpack(NULL, pbrequest->length, pbrequest->buffer);

	if (!request)
		return PBRPC_BAD_REQEST_DATA;

	connection = app_context_get_connection(request->connectionid);

	if (connection)
	{
		FDSAPI_LOGOFF_USER_REQUEST* pRequest;

		pRequest = (FDSAPI_LOGOFF_USER_REQUEST*) malloc(sizeof(FDSAPI_LOGOFF_USER_REQUEST));

		pRequest->callId = tag;
		pRequest->msgType = FDSAPI_LOGOFF_USER_REQUEST_ID;
		freerds__icp__log_off_user_session_request__free_unpacked(request, NULL);

		MessageQueue_Post(connection->notifications, (void*) connection, pRequest->msgType, (void*) pRequest, NULL);

		*pbresponse = NULL;
		return 0;
	}
	else
	{
		fprintf(stderr, "something went wrong\n");
		response.loggedoff = FALSE;
	}

	freerds__icp__log_off_user_session_request__free_unpacked(request, NULL);

	payload = pbrpc_payload_new();
	payload->length = freerds__icp__log_off_user_session_response__get_packed_size(&response);
	payload->buffer = malloc(payload->length);

	ret = freerds__icp__log_off_user_session_response__pack(&response, payload->buffer);

	if (ret != payload->length)
	{
		free(payload->buffer);
		return PBRPC_BAD_RESPONSE;
	}
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}
