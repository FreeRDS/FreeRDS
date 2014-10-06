/**
 * FreeRDS internal communication protocol
 * client stub implementation
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
#include <freerds/icp_client_stubs.h>

#include <winpr/crt.h>

#include "pbrpc.h"
#include "../core/core.h"

int freerds_icp_LogoffUserResponse(FDSAPI_LOGOFF_USER_RESPONSE* pResponse)
{
	pbRPCPayload* pbresponse;
	UINT32 type = FDSAPI_LOGOFF_USER_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return -1;

	pResponse->status = 0;
	pResponse->msgType = FDSAPI_LOGOFF_USER_RESPONSE_ID;

	pbresponse = pbrpc_payload_new();
	pbresponse->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), pResponse, NULL);
	pbresponse->buffer = Stream_Buffer(pbresponse->s);
	pbresponse->length = Stream_Length(pbresponse->s);

	return pbrpc_send_response(context, pbresponse, pResponse->status, pResponse->msgType, pResponse->callId);
}

int freerds_icp_SwitchServiceEndpointResponse(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE* pResponse)
{
	pbRPCPayload* pbresponse;
	UINT32 type = FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return -1;

	pResponse->status = 0;
	pResponse->msgType = FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE_ID;

	pbresponse = pbrpc_payload_new();
	pbresponse->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), pResponse, NULL);
	pbresponse->buffer = Stream_Buffer(pbresponse->s);
	pbresponse->length = Stream_Length(pbresponse->s);

	return pbrpc_send_response(context, pbresponse, pResponse->status, pResponse->msgType, pResponse->callId);
}

int freerds_icp_IsChannelAllowed(FDSAPI_CHANNEL_ALLOWED_REQUEST* pRequest, FDSAPI_CHANNEL_ALLOWED_RESPONSE* pResponse)
{
	int status;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	UINT32 type = FDSAPI_CHANNEL_ALLOWED_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	pbrequest.s = freerds_rpc_msg_pack(type, pRequest, NULL);
	pbrequest.buffer = Stream_Buffer(pbrequest.s);
	pbrequest.length = Stream_Length(pbrequest.s);

	status = pbrpc_call_method(context, FDSAPI_REQUEST_ID(type), &pbrequest, &pbresponse);

	Stream_Free(pbrequest.s, TRUE);

	if (status != 0)
		return status;

	freerds_rpc_msg_unpack(FDSAPI_RESPONSE_ID(type), pResponse, pbresponse->buffer, pbresponse->length);

	pbrpc_free_payload(pbresponse);

	return PBRPC_SUCCESS;
}

int freerds_icp_DisconnectUserSession(FDSAPI_DISCONNECT_USER_REQUEST* pRequest, FDSAPI_DISCONNECT_USER_RESPONSE* pResponse)
{
	int status;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	UINT32 type = FDSAPI_DISCONNECT_USER_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	pbrequest.s = freerds_rpc_msg_pack(type, pRequest, NULL);
	pbrequest.buffer = Stream_Buffer(pbrequest.s);
	pbrequest.length = Stream_Length(pbrequest.s);

	status = pbrpc_call_method(context, FDSAPI_REQUEST_ID(type), &pbrequest, &pbresponse);

	Stream_Free(pbrequest.s, TRUE);

	if (status != 0)
		return status;

	freerds_rpc_msg_unpack(FDSAPI_RESPONSE_ID(type), pResponse, pbresponse->buffer, pbresponse->length);

	pbrpc_free_payload(pbresponse);

	return PBRPC_SUCCESS;
}

int freerds_icp_LogOffUserSession(FDSAPI_LOGOFF_USER_REQUEST* pRequest, FDSAPI_LOGOFF_USER_RESPONSE* pResponse)
{
	int status;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	UINT32 type = FDSAPI_LOGOFF_USER_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	pbrequest.s = freerds_rpc_msg_pack(type, pRequest, NULL);
	pbrequest.buffer = Stream_Buffer(pbrequest.s);
	pbrequest.length = Stream_Length(pbrequest.s);

	status = pbrpc_call_method(context, FDSAPI_REQUEST_ID(type), &pbrequest, &pbresponse);

	Stream_Free(pbrequest.s, TRUE);

	if (status != 0)
		return status;

	freerds_rpc_msg_unpack(FDSAPI_RESPONSE_ID(type), pResponse, pbresponse->buffer, pbresponse->length);

	pbrpc_free_payload(pbresponse);

	return PBRPC_SUCCESS;
}

int freerds_icp_LogonUser(FDSAPI_LOGON_USER_REQUEST* pRequest, FDSAPI_LOGON_USER_RESPONSE* pResponse)
{
	int status;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	UINT32 type = FDSAPI_LOGON_USER_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	pbrequest.s = freerds_rpc_msg_pack(type, pRequest, NULL);
	pbrequest.buffer = Stream_Buffer(pbrequest.s);
	pbrequest.length = Stream_Length(pbrequest.s);

	status = pbrpc_call_method(context, FDSAPI_REQUEST_ID(type), &pbrequest, &pbresponse);

	Stream_Free(pbrequest.s, TRUE);

	if (status != 0)
		return status;

	freerds_rpc_msg_unpack(FDSAPI_RESPONSE_ID(type), pResponse, pbresponse->buffer, pbresponse->length);

	pbrpc_free_payload(pbresponse);

	return PBRPC_SUCCESS;
}