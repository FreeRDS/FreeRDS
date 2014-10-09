/**
 * pbRPC - a simple, protocol buffer based RCP mechanism
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

#ifndef _PBRPC_H
#define _PBRPC_H

#include <winpr/synch.h>
#include <winpr/stream.h>
#include <winpr/collections.h>

#include <freerds/rpc.h>

#include "freerds.h"

struct pbrpc_context
{
	LONG tag;
	HANDLE hPipe;
	HANDLE stopEvent;
	HANDLE thread;
	BOOL isConnected;
	wQueue* writeQueue;
	wListDictionary* transactions;
};

struct pbrpc_payload
{
	wStream* s;
	BYTE* buffer;
	UINT32 length;
};
typedef struct pbrpc_payload pbRPCPayload;

typedef int (*pbRPCCallback)(FDSAPI_MSG_PACKET* msg, pbRPCPayload** pbresponse);

typedef enum pbrpc_status
{
	PBRPC_SUCCESS = 0,
	PBRPC_FAILED = 1,
	PBRPC_NOTFOUND = 2,
	PBRPC_BAD_REQEST_DATA = 100,
	PBRPC_BAD_RESPONSE = 101,
	PBRCP_TRANSPORT_ERROR = 102,
	PBRCP_CALL_TIMEOUT = 103
} PBRPCSTATUS;

typedef void (*pbRpcResponseCallback)(UINT32 reason, FDSAPI_MSG_PACKET* response, void *args);

int pbrpc_server_start(pbRPCContext* context);
int pbrpc_server_stop(pbRPCContext* context);

pbRPCContext* pbrpc_server_new();
void pbrpc_server_free(pbRPCContext* context);

/* icp */

int freerds_icp_IsChannelAllowed(FDSAPI_CHANNEL_ALLOWED_REQUEST* pRequest, FDSAPI_CHANNEL_ALLOWED_RESPONSE* pResponse);
int freerds_icp_DisconnectUserSession(FDSAPI_DISCONNECT_USER_REQUEST* pRequest, FDSAPI_DISCONNECT_USER_RESPONSE* pResponse);
int freerds_icp_LogOffUserSession(FDSAPI_LOGOFF_USER_REQUEST* pRequest, FDSAPI_LOGOFF_USER_RESPONSE* pResponse);
int freerds_icp_LogonUser(FDSAPI_LOGON_USER_REQUEST* pRequest, FDSAPI_LOGON_USER_RESPONSE* pResponse);

int freerds_icp_Heartbeat(FDSAPI_MSG_PACKET* msg, pbRPCPayload** pbresponse);

int freerds_icp_SwitchServiceEndpointResponse(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE* pResponse);
int freerds_icp_SwitchServiceEndpoint(FDSAPI_MSG_PACKET* msg, pbRPCPayload** pbresponse);

int freerds_icp_LogoffUserResponse(FDSAPI_LOGOFF_USER_RESPONSE* pResponse);
int freerds_icp_LogoffUser(FDSAPI_MSG_PACKET* msg, pbRPCPayload** pbresponse);

int freerds_icp_ChannelEndpointOpenResponse(FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE* pResponse);
int freerds_icp_ChannelEndpointOpen(FDSAPI_MSG_PACKET* msg, pbRPCPayload** pbresponse);

#endif //_PBRPC_H
