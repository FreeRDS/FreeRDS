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

#include "ICP.pb-c.h"
#include "pbrpc.h"
#include "../core/core.h"

int freerds_icp_LogoffUserResponse(FDSAPI_LOGOFF_USER_RESPONSE* pResponse)
{
	int status = 0;
	Freerds__Icp__LogOffUserSessionResponse response;
	pbRPCPayload* pbresponse = pbrpc_payload_new();
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return -1;

	//pResponse->msgType = FDSAPI_LOGOFF_USER_RESPONSE_ID;
	pResponse->msgType = FREERDS__ICP__MSGTYPE__LogOffUserSession;

	freerds__icp__log_off_user_session_response__init(&response);
	response.loggedoff = pResponse->status ? FALSE : TRUE;

	pbresponse->length = freerds__icp__log_off_user_session_response__get_packed_size(&response);
	pbresponse->buffer = (BYTE*) malloc(pbresponse->length);

	status = freerds__icp__log_off_user_session_response__pack(&response, pbresponse->buffer);

	if (status != pbresponse->length)
	{
		fprintf(stderr, "%s pack error for %d", __FUNCTION__, pResponse->msgType);
		pbrpc_free_payload(pbresponse);
		return -1;
	}

	return pbrpc_send_response(context, pbresponse, pResponse->status, pResponse->msgType, pResponse->callId);
}

int freerds_icp_SwitchServiceEndpointResponse(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE* pResponse)
{
	int status = 0;
	Freerds__Icp__SwitchToResponse response;
	pbRPCPayload* pbresponse = pbrpc_payload_new();
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return -1;

	//pResponse->msgType = FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE_ID;
	pResponse->msgType = FREERDS__ICP__MSGTYPE__SwitchTo;

	freerds__icp__switch_to_response__init(&response);
	response.success = pResponse->status ? FALSE : TRUE;

	pbresponse->length = freerds__icp__switch_to_response__get_packed_size(&response);
	pbresponse->buffer = (BYTE*) malloc(pbresponse->length);

	status = freerds__icp__switch_to_response__pack(&response, pbresponse->buffer);

	if (status != pbresponse->length)
	{
		fprintf(stderr, "%s pack error for %d", __FUNCTION__, pResponse->msgType);
		pbrpc_free_payload(pbresponse);
		return -1;
	}

	return pbrpc_send_response(context, pbresponse, pResponse->status, pResponse->msgType, pResponse->callId);
}

int freerds_icp_IsChannelAllowed(FDSAPI_CHANNEL_ALLOWED_REQUEST* pRequest, FDSAPI_CHANNEL_ALLOWED_RESPONSE* pResponse)
{
	int ret;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	Freerds__Icp__IsChannelAllowedRequest request;
	Freerds__Icp__IsChannelAllowedResponse* response = NULL;
	UINT32 type = FREERDS__ICP__MSGTYPE__IsChannelAllowed;

	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	freerds__icp__is_channel_allowed_request__init(&request);

	request.channelname = pRequest->ChannelName;

	pbrequest.length = freerds__icp__is_channel_allowed_request__get_packed_size(&request);
	pbrequest.buffer = (BYTE*) malloc(pbrequest.length);
	ret = freerds__icp__is_channel_allowed_request__pack(&request, pbrequest.buffer);
	if (ret == pbrequest.length)
		ret = pbrpc_call_method(context, type, &pbrequest, &pbresponse);
	else
		ret = PBRPC_BAD_REQEST_DATA;
	free(pbrequest.buffer);

	if (ret != 0)
		return ret;

	response = freerds__icp__is_channel_allowed_response__unpack(NULL, pbresponse->length, pbresponse->buffer);
	pbrpc_free_payload(pbresponse);

	if (!response)
		return PBRPC_BAD_RESPONSE;

	pResponse->ChannelAllowed = response->channelallowed ? TRUE : FALSE;

	freerds__icp__is_channel_allowed_response__free_unpacked(response, NULL);

	return PBRPC_SUCCESS;
}

int freerds_icp_DisconnectUserSession(FDSAPI_DISCONNECT_USER_REQUEST* pRequest, FDSAPI_DISCONNECT_USER_RESPONSE* pResponse)
{
	int ret;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	Freerds__Icp__DisconnectUserSessionRequest request;
	Freerds__Icp__DisconnectUserSessionResponse* response = NULL;
	UINT32 type = FREERDS__ICP__MSGTYPE__DisconnectUserSession;

	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	freerds__icp__disconnect_user_session_request__init(&request);

	request.connectionid = pRequest->ConnectionId;

	pbrequest.length = freerds__icp__disconnect_user_session_request__get_packed_size(&request);
	pbrequest.buffer = (BYTE*) malloc(pbrequest.length);
	ret = freerds__icp__disconnect_user_session_request__pack(&request, pbrequest.buffer);

	if (ret == pbrequest.length)
		ret = pbrpc_call_method(context, type, &pbrequest, &pbresponse);
	else
		ret = PBRPC_BAD_REQEST_DATA;

	free(pbrequest.buffer);

	if (ret != 0)
		return ret;

	response = freerds__icp__disconnect_user_session_response__unpack(NULL, pbresponse->length, pbresponse->buffer);
	pbrpc_free_payload(pbresponse);

	if (!response)
		return PBRPC_BAD_RESPONSE;

	pResponse->ConnectionId = pRequest->ConnectionId;

	freerds__icp__disconnect_user_session_response__free_unpacked(response, NULL);

	return PBRPC_SUCCESS;
}

int freerds_icp_LogOffUserSession(FDSAPI_LOGOFF_USER_REQUEST* pRequest, FDSAPI_LOGOFF_USER_RESPONSE* pResponse)
{
	int ret;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	Freerds__Icp__LogOffUserSessionRequest request;
	Freerds__Icp__LogOffUserSessionResponse* response = NULL;
	UINT32 type = FREERDS__ICP__MSGTYPE__LogOffUserSession;

	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	freerds__icp__log_off_user_session_request__init(&request);

	request.connectionid = pRequest->ConnectionId;

	pbrequest.length = freerds__icp__log_off_user_session_request__get_packed_size(&request);
	pbrequest.buffer = (BYTE*) malloc(pbrequest.length);
	ret = freerds__icp__log_off_user_session_request__pack(&request, pbrequest.buffer);
	if (ret == pbrequest.length)
		ret = pbrpc_call_method(context, type, &pbrequest, &pbresponse);
	else
		ret = PBRPC_BAD_REQEST_DATA;
	free(pbrequest.buffer);

	if (ret != 0)
		return ret;

	response = freerds__icp__log_off_user_session_response__unpack(NULL, pbresponse->length, pbresponse->buffer);
	pbrpc_free_payload(pbresponse);

	if (!response)
		return PBRPC_BAD_RESPONSE;

	pResponse->ConnectionId = pRequest->ConnectionId;

	freerds__icp__log_off_user_session_response__free_unpacked(response, NULL);

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
