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

#define ICP_CLIENT_STUB_SETUP(camel, expanded) \
	UINT32 type = FREERDS__ICP__MSGTYPE__##camel ; \
	pbRPCPayload pbrequest; \
	pbRPCPayload *pbresponse = NULL; \
	int ret; \
	Freerds__Icp__##camel ## Request request; \
	Freerds__Icp__##camel ## Response *response = NULL; \
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context(); \
	if (!context) \
		return PBRPC_FAILED; \
	freerds__icp__ ##expanded ##_request__init(&request);

#define ICP_CLIENT_STUB_CALL(camel, expanded) \
	pbrequest.dataLen = freerds__icp__##expanded ##_request__get_packed_size(&request); \
	pbrequest.data = malloc(pbrequest.dataLen); \
	ret = freerds__icp__##expanded ##_request__pack(&request, (uint8_t*) pbrequest.data); \
	if (ret == pbrequest.dataLen) \
	{ \
		ret = pbrpc_call_method(context, type, &pbrequest, &pbresponse); \
	} \
	else \
	{ \
		ret = PBRPC_BAD_REQEST_DATA; \
	} \
	free(pbrequest.data);

#define ICP_CLIENT_STUB_UNPACK_RESPONSE(camel, expanded) \
	response = freerds__icp__##expanded ##_response__unpack(NULL, pbresponse->dataLen, (uint8_t*) pbresponse->data); \
	pbrpc_free_payload(pbresponse);

#define ICP_CLIENT_STUB_CLEANUP(camel, expanded) \
	freerds__icp__##expanded ##_response__free_unpacked(response, NULL);

#define ICP_CLIENT_SEND_PREPARE(camel, expanded) \
			Freerds__Icp__##camel ##Response response; \
			freerds__icp__##expanded ##_response__init(&response);

#define ICP_CLIENT_SEND_PACK(camel, expanded) \
			pbresponse->dataLen = freerds__icp__##expanded ##_response__get_packed_size(&response); \
			pbresponse->data = malloc(pbresponse->dataLen); \
			ret = freerds__icp__##expanded ##_response__pack(&response, (uint8_t*) pbresponse->data);

int freerds_icp_sendResponse(UINT32 tag, UINT32 type, UINT32 status, BOOL success)
{
	int rtype = 0;
	pbRPCPayload *pbresponse = pbrpc_payload_new();
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();
	int ret = 0;

	if (!context)
		return -1;

	switch (type)
	{
		case NOTIFY_SWITCHTO:
			{
				rtype = FREERDS__ICP__MSGTYPE__SwitchTo;
				ICP_CLIENT_SEND_PREPARE(SwitchTo, switch_to)
				response.success = success;
				ICP_CLIENT_SEND_PACK(SwitchTo, switch_to)
			}
			break;
		case NOTIFY_LOGOFF:
			{
				rtype = FREERDS__ICP__MSGTYPE__LogOffUserSession;
				ICP_CLIENT_SEND_PREPARE(LogOffUserSession, log_off_user_session)
				response.loggedoff = success;
				ICP_CLIENT_SEND_PACK(LogOffUserSession, log_off_user_session)
			}
			break;
		default:
			/* type not found */
			return -1;
			break;
	}

	if (ret != pbresponse->dataLen)
	{
		fprintf(stderr, "%s pack error for %d", __FUNCTION__, type);
		pbrpc_free_payload(pbresponse);
		return -1;
	}

	return  pbrpc_send_response(context, pbresponse, status, rtype, tag);
}

int freerds_icp_IsChannelAllowed(FDSAPI_CHANNEL_ALLOWED_REQUEST* pRequest, FDSAPI_CHANNEL_ALLOWED_RESPONSE* pResponse)
{
	ICP_CLIENT_STUB_SETUP(IsChannelAllowed, is_channel_allowed)

	request.channelname = pRequest->ChannelName;

	ICP_CLIENT_STUB_CALL(IsChannelAllowed, is_channel_allowed)

	if (ret != 0)
		return ret;

	ICP_CLIENT_STUB_UNPACK_RESPONSE(IsChannelAllowed, is_channel_allowed)

	if (!response)
		return PBRPC_BAD_RESPONSE;

	pResponse->ChannelAllowed = response->channelallowed ? TRUE : FALSE;

	ICP_CLIENT_STUB_CLEANUP(IsChannelAllowed, is_channel_allowed)

	return PBRPC_SUCCESS;
}

int freerds_icp_DisconnectUserSession(UINT32 connectionId, BOOL* disconnected)
{
	ICP_CLIENT_STUB_SETUP(DisconnectUserSession, disconnect_user_session)

	request.connectionid = connectionId;

	ICP_CLIENT_STUB_CALL(DisconnectUserSession, disconnect_user_session)

	if (ret != 0)
		return ret;

	ICP_CLIENT_STUB_UNPACK_RESPONSE(DisconnectUserSession, disconnect_user_session)

	if (!response)
		return PBRPC_BAD_RESPONSE;

	*disconnected = response->disconnected;

	ICP_CLIENT_STUB_CLEANUP(DisconnectUserSession, disconnect_user_session)

	return PBRPC_SUCCESS;
}

int freerds_icp_LogOffUserSession(UINT32 connectionId, BOOL* loggedoff)
{
	ICP_CLIENT_STUB_SETUP(LogOffUserSession, log_off_user_session)

	request.connectionid = connectionId;

	ICP_CLIENT_STUB_CALL(LogOffUserSession, log_off_user_session)

	if (ret != 0)
		return ret;

	ICP_CLIENT_STUB_UNPACK_RESPONSE(LogOffUserSession, log_off_user_session)

	if (!response)
		return PBRPC_BAD_RESPONSE;

	*loggedoff = response->loggedoff;

	ICP_CLIENT_STUB_CLEANUP(LogOffUserSession, log_off_user_session)

	return PBRPC_SUCCESS;
}

int freerds_icp_LogonUser(FDSAPI_LOGON_USER_REQUEST* pRequest, FDSAPI_LOGON_USER_RESPONSE* pResponse)
{
	ICP_CLIENT_STUB_SETUP(LogonUser, logon_user)

	request.connectionid = pRequest->ConnectionId;
	request.domain = pRequest->Domain;
	request.username = pRequest->User;
	request.password = pRequest->Password;
	request.width = pRequest->DesktopWidth;
	request.height = pRequest->DesktopHeight;
	request.colordepth = pRequest->ColorDepth;
	request.clientname = pRequest->ClientName;
	request.clientaddress = pRequest->ClientAddress;
	request.clientbuildnumber = pRequest->ClientBuild;
	request.clientproductid = pRequest->ClientProductId;
	request.clienthardwareid = pRequest->ClientHardwareId;
	request.clientprotocoltype = pRequest->ClientProtocolType;

	ICP_CLIENT_STUB_CALL(LogonUser, logon_user)

	if (ret != 0)
		return ret;

	ICP_CLIENT_STUB_UNPACK_RESPONSE(LogonUser, logon_user)

	if (!response)
		return PBRPC_BAD_RESPONSE;

	pResponse->ServiceEndpoint = _strdup(response->serviceendpoint);

	ICP_CLIENT_STUB_CLEANUP(LogonUser, logon_user)

	return PBRPC_SUCCESS;
}
