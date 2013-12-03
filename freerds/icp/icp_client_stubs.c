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
#include "pbrpc_utils.h"
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

int freerds_icp_IsChannelAllowed(UINT32 connectionId, char* channelName, BOOL* isAllowed)
{
	ICP_CLIENT_STUB_SETUP(IsChannelAllowed, is_channel_allowed)

	// set call specific parameters
	request.channelname = channelName;

	ICP_CLIENT_STUB_CALL(IsChannelAllowed, is_channel_allowed)

	if (ret != 0)
	{
		// handle function specific frees
		return ret;
	}

	ICP_CLIENT_STUB_UNPACK_RESPONSE(IsChannelAllowed, is_channel_allowed)

	if (NULL == response)
	{
		// unpack error
		// free function specific stuff
		return PBRPC_BAD_RESPONSE;
	}

	// assign returned stuff here!
	// don't use pointers since response get's freed (copy might be required..)
	*isAllowed = response->channelallowed;

	// free function specific stuff

	ICP_CLIENT_STUB_CLEANUP(IsChannelAllowed, is_channel_allowed)

	return PBRPC_SUCCESS;
}

int freerds_icp_Ping(BOOL* pong)
{
	ICP_CLIENT_STUB_SETUP(Ping, ping)

	ICP_CLIENT_STUB_CALL(Ping, ping)

	if (ret != 0)
	{
		// handle function specific frees
		return ret;
	}

	ICP_CLIENT_STUB_UNPACK_RESPONSE(Ping, ping)

	if (NULL == response)
	{
		// unpack error
		// free function specific stuff
		return PBRPC_BAD_RESPONSE;
	}

	// assign returned stuff here!
	// don't use pointers since response get's freed (copy might be required..)
	*pong = response->pong;

	// free function specific stuff

	ICP_CLIENT_STUB_CLEANUP(Ping, ping)

	return PBRPC_SUCCESS;
}

int freerds_icp_DisconnectUserSession(UINT32 connectionId, BOOL* disconnected)
{
	ICP_CLIENT_STUB_SETUP(DisconnectUserSession, disconnect_user_session)

	request.connectionid = connectionId;

	ICP_CLIENT_STUB_CALL(DisconnectUserSession, disconnect_user_session)

	if (ret != 0)
	{
		// handle function specific frees
		return ret;
	}

	ICP_CLIENT_STUB_UNPACK_RESPONSE(DisconnectUserSession, disconnect_user_session)

	if (NULL == response)
	{
		// unpack error
		return PBRPC_BAD_RESPONSE;
	}

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
	{
		// handle function specific frees
		return ret;
	}

	ICP_CLIENT_STUB_UNPACK_RESPONSE(LogOffUserSession, log_off_user_session)

	if (NULL == response)
	{
		// unpack error
		return PBRPC_BAD_RESPONSE;
	}

	*loggedoff = response->loggedoff;

	ICP_CLIENT_STUB_CLEANUP(LogOffUserSession, log_off_user_session)

	return PBRPC_SUCCESS;
}

int freerds_icp_LogonUser(UINT32 connectionId, char* username, char* domain,
		char* password, UINT32 width, UINT32 height, UINT32 bbp, char** serviceEndpoint)
{
	ICP_CLIENT_STUB_SETUP(LogonUser, logon_user)

	request.connectionid = connectionId;
	request.domain = domain;
	request.username = username;
	request.password = password;
	request.width = width;
	request.height = height;
	request.colordepth = bbp;

	ICP_CLIENT_STUB_CALL(LogonUser, logon_user)

	if (ret != 0)
	{
		// handle function specific frees
		return ret;
	}

	ICP_CLIENT_STUB_UNPACK_RESPONSE(LogonUser, logon_user)

	if (NULL == response)
	{
		// unpack error
		// free function specific stuff
		return PBRPC_BAD_RESPONSE;
	}

	// assign returned stuff here!
	// don't use pointers since response get's freed (copy might be required..)
	*serviceEndpoint = _strdup(response->serviceendpoint);

	// free function specific stuff

	ICP_CLIENT_STUB_CLEANUP(LogonUser, logon_user)

	return PBRPC_SUCCESS;
}
