/**
 * FreeRDS internal communication protocol
 * client stub implementation
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

#include <freerds/icp.h>
#include <freerds/icp_client_stubs.h>

#include <winpr/crt.h>

#include "ICP.pb-c.h"
#include "pbrpc.h"
#include "pbrpc_utils.h"

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


int freerds_icp_IsChannelAllowed(int sessionId, char* channelName, BOOL* isAllowed)
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

int freerds_icp_GetUserSession(char* username, char* domain, UINT32* sessionID, char** serviceEndpoint)
{
	ICP_CLIENT_STUB_SETUP(GetUserSession, get_user_session)

	request.domainname = domain;
	request.username = username;

	ICP_CLIENT_STUB_CALL(GetUserSession, get_user_session)

	if (ret != 0)
	{
		// handle function specific frees
		return ret;
	}

	ICP_CLIENT_STUB_UNPACK_RESPONSE(GetUserSession, get_user_session)

	if (NULL == response)
	{
		// unpack error
		// free function specific stuff
		return PBRPC_BAD_RESPONSE;
	}

	// assign returned stuff here!
	// don't use pointers since response get's freed (copy might be required..)
	*sessionID = response->sessionid;
	*serviceEndpoint = _strdup(response->serviceendpoint);

	// free function specific stuff

	ICP_CLIENT_STUB_CLEANUP(GetUserSession, get_user_session)

	return PBRPC_SUCCESS;
}

int freerds_icp_DisconnectUserSession(UINT32 sessionID, BOOL* disconnected)
{
	ICP_CLIENT_STUB_SETUP(DisconnectUserSession, disconnect_user_session)

	request.sessionid = sessionID;

	ICP_CLIENT_STUB_CALL(GetUserSession, disconnect_user_session)

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

int freerds_icp_LogOffUserSession(UINT32 sessionID, BOOL* loggedoff)
{
	ICP_CLIENT_STUB_SETUP(LogOffUserSession, log_off_user_session)

	request.sessionid = sessionID;

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

int freerds_icp_LogonUser(char* username, char* domain, char *password, UINT32* authStatus, char** serviceEndpoint)
{
	ICP_CLIENT_STUB_SETUP(LogonUser, logon_user)

	request.domain = domain;
	request.username = username;
	request.password = password;

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
	*authStatus = response->authstatus;
	*serviceEndpoint = _strdup(response->serviceendpoint);

	// free function specific stuff

	ICP_CLIENT_STUB_CLEANUP(LogonUser, logon_user)

	return PBRPC_SUCCESS;
}
