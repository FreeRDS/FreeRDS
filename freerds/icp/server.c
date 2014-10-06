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

#include "pbrpc.h"
#include "../core/app_context.h"

int freerds_icp_Heartbeat(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	pbRPCPayload* payload;
	FDSAPI_HEARTBEAT_REQUEST request;
	FDSAPI_HEARTBEAT_RESPONSE response;
	UINT32 type = FDSAPI_HEARTBEAT_REQUEST_ID;

	freerds_rpc_msg_unpack(FDSAPI_REQUEST_ID(type), &request, pbrequest->buffer, pbrequest->length);

	response.msgType = FDSAPI_RESPONSE_ID(type);
	response.callId = request.callId;
	response.HeartbeatId = request.HeartbeatId;

	payload = pbrpc_payload_new();
	payload->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), &response, NULL);
	payload->buffer = Stream_Buffer(payload->s);
	payload->length = Stream_Length(payload->s);
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

int freerds_icp_SwitchServiceEndpoint(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	pbRPCPayload* payload;
	rdsConnection* connection = NULL;
	FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST request;
	FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE response;
	UINT32 type = FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID;

	freerds_rpc_msg_unpack(FDSAPI_REQUEST_ID(type), &request, pbrequest->buffer, pbrequest->length);

	response.msgType = FDSAPI_RESPONSE_ID(type);
	response.callId = request.callId;

	connection = app_context_get_connection(request.ConnectionId);

	if (connection)
	{
		FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST* pRequest;

		pRequest = (FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST*) malloc(sizeof(FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST));

		pRequest->callId = tag;
		pRequest->msgType = FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID;
		pRequest->ServiceEndpoint = _strdup(request.ServiceEndpoint);

		MessageQueue_Post(connection->notifications, (void*) connection, pRequest->msgType, (void*) pRequest, NULL);
		*pbresponse = NULL;
		return 0;
	}
	else
	{
		fprintf(stderr, "something went wrong\n");
		response.status = 1;
	}

	payload = pbrpc_payload_new();
	payload->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), &response, NULL);
	payload->buffer = Stream_Buffer(payload->s);
	payload->length = Stream_Length(payload->s);
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

int freerds_icp_LogoffUser(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	pbRPCPayload* payload;
	rdsConnection* connection = NULL;
	FDSAPI_LOGOFF_USER_REQUEST request;
	FDSAPI_LOGOFF_USER_RESPONSE response;
	UINT32 type = FDSAPI_LOGOFF_USER_REQUEST_ID;

	freerds_rpc_msg_unpack(FDSAPI_REQUEST_ID(type), &request, pbrequest->buffer, pbrequest->length);

	response.msgType = FDSAPI_RESPONSE_ID(type);
	response.callId = request.callId;

	connection = app_context_get_connection(request.ConnectionId);

	if (connection)
	{
		FDSAPI_LOGOFF_USER_REQUEST* pRequest;

		pRequest = (FDSAPI_LOGOFF_USER_REQUEST*) malloc(sizeof(FDSAPI_LOGOFF_USER_REQUEST));

		pRequest->callId = tag;
		pRequest->msgType = FDSAPI_LOGOFF_USER_REQUEST_ID;

		MessageQueue_Post(connection->notifications, (void*) connection, pRequest->msgType, (void*) pRequest, NULL);

		*pbresponse = NULL;
		return 0;
	}
	else
	{
		fprintf(stderr, "something went wrong\n");
		response.status = 1;
	}

	payload = pbrpc_payload_new();
	payload->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), &response, NULL);
	payload->buffer = Stream_Buffer(payload->s);
	payload->length = Stream_Length(payload->s);
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

int freerds_icp_ChannelEndpointOpen(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	pbRPCPayload* payload;
	FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST request;
	FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE response;
	UINT32 type = FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST_ID;

	freerds_rpc_msg_unpack(FDSAPI_REQUEST_ID(type), &request, pbrequest->buffer, pbrequest->length);

	response.msgType = FDSAPI_RESPONSE_ID(type);
	response.callId = request.callId;
	response.status = 0;

	payload = pbrpc_payload_new();
	payload->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), &response, NULL);
	payload->buffer = Stream_Buffer(payload->s);
	payload->length = Stream_Length(payload->s);
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}
