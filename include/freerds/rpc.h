/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * Remote Procedure Call (RPC) System
 *
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#ifndef FREERDS_RPC_H
#define FREERDS_RPC_H

#include <freerds/api.h>

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/wtypes.h>
#include <winpr/stream.h>
#include <winpr/collections.h>

#define FDSAPI_MSG_HEADER_SIZE	16

#define FDSAPI_MSG_COMMON() \
	UINT32 msgSize; \
	UINT32 msgType; \
	UINT32 callId; \
	UINT32 status

struct _FDSAPI_MSG_HEADER
{
	FDSAPI_MSG_COMMON();
};
typedef struct _FDSAPI_MSG_HEADER FDSAPI_MSG_HEADER;

struct _FDSAPI_MSG_PACKET
{
	FDSAPI_MSG_COMMON();

	BYTE* buffer;
	UINT32 length;
};
typedef struct _FDSAPI_MSG_PACKET FDSAPI_MSG_PACKET;

/* RPC Status Code Definitions */

#define FDSAPI_STATUS_SUCCESS		0
#define FDSAPI_STATUS_FAILED		1
#define FDSAPI_STATUS_NOTFOUND		2

/* Message Type Id Macros */

#define FDSAPI_REQUEST_ID(_id)		(_id & ~(0x80000000))
#define FDSAPI_RESPONSE_ID(_id)		(_id | 0x80000000)
#define FDSAPI_IS_RESPONSE_ID(_id)	(_id & 0x80000000)

/* RPC Message Definitions */

#define FDSAPI_CHANNEL_ALLOWED_REQUEST_ID			FDSAPI_REQUEST_ID(1001)
#define FDSAPI_CHANNEL_ALLOWED_RESPONSE_ID			FDSAPI_RESPONSE_ID(1001)
#define FDSAPI_HEARTBEAT_REQUEST_ID				FDSAPI_REQUEST_ID(1002)
#define FDSAPI_HEARTBEAT_RESPONSE_ID				FDSAPI_RESPONSE_ID(1002)
#define FDSAPI_LOGON_USER_REQUEST_ID				FDSAPI_REQUEST_ID(1003)
#define FDSAPI_LOGON_USER_RESPONSE_ID				FDSAPI_RESPONSE_ID(1003)
#define FDSAPI_LOGOFF_USER_REQUEST_ID				FDSAPI_REQUEST_ID(1004)
#define FDSAPI_LOGOFF_USER_RESPONSE_ID				FDSAPI_RESPONSE_ID(1004)
#define FDSAPI_DISCONNECT_USER_REQUEST_ID			FDSAPI_REQUEST_ID(1005)
#define FDSAPI_DISCONNECT_USER_RESPONSE_ID			FDSAPI_RESPONSE_ID(1005)
#define FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID		FDSAPI_REQUEST_ID(1006)
#define FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE_ID		FDSAPI_RESPONSE_ID(1006)
#define FDSAPI_START_SESSION_REQUEST_ID				FDSAPI_REQUEST_ID(1007)
#define FDSAPI_START_SESSION_RESPONSE_ID			FDSAPI_RESPONSE_ID(1007)
#define FDSAPI_END_SESSION_REQUEST_ID				FDSAPI_REQUEST_ID(1008)
#define FDSAPI_END_SESSION_RESPONSE_ID				FDSAPI_RESPONSE_ID(1008)
#define FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST_ID			FDSAPI_REQUEST_ID(1009)
#define FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE_ID		FDSAPI_RESPONSE_ID(1009)

struct _FDSAPI_CHANNEL_ALLOWED_REQUEST
{
	FDSAPI_MSG_COMMON();
	char* ChannelName;
};
typedef struct _FDSAPI_CHANNEL_ALLOWED_REQUEST FDSAPI_CHANNEL_ALLOWED_REQUEST;

struct _FDSAPI_CHANNEL_ALLOWED_RESPONSE
{
	FDSAPI_MSG_COMMON();
	BOOL ChannelAllowed;
};
typedef struct _FDSAPI_CHANNEL_ALLOWED_RESPONSE FDSAPI_CHANNEL_ALLOWED_RESPONSE;

struct _FDSAPI_HEARTBEAT_REQUEST
{
	FDSAPI_MSG_COMMON();
	UINT32 HeartbeatId;
};
typedef struct _FDSAPI_HEARTBEAT_REQUEST FDSAPI_HEARTBEAT_REQUEST;

struct _FDSAPI_HEARTBEAT_RESPONSE
{
	FDSAPI_MSG_COMMON();
	UINT32 HeartbeatId;
};
typedef struct _FDSAPI_HEARTBEAT_RESPONSE FDSAPI_HEARTBEAT_RESPONSE;

struct _FDSAPI_LOGON_USER_REQUEST
{
	FDSAPI_MSG_COMMON();
	UINT32 ConnectionId;
	char* User;
	char* Domain;
	char* Password;
	UINT32 DesktopWidth;
	UINT32 DesktopHeight;
	UINT32 ColorDepth;
	char* ClientName;
	char* ClientAddress;
	UINT32 ClientBuild;
	UINT32 ClientProductId;
	UINT32 ClientHardwareId;
	UINT32 ClientProtocolType;
};
typedef struct _FDSAPI_LOGON_USER_REQUEST FDSAPI_LOGON_USER_REQUEST;

struct _FDSAPI_LOGON_USER_RESPONSE
{
	FDSAPI_MSG_COMMON();
	char* ServiceEndpoint;
};
typedef struct _FDSAPI_LOGON_USER_RESPONSE FDSAPI_LOGON_USER_RESPONSE;

struct _FDSAPI_LOGOFF_USER_REQUEST
{
	FDSAPI_MSG_COMMON();
	UINT32 ConnectionId;
};
typedef struct _FDSAPI_LOGOFF_USER_REQUEST FDSAPI_LOGOFF_USER_REQUEST;

struct _FDSAPI_LOGOFF_USER_RESPONSE
{
	FDSAPI_MSG_COMMON();
	UINT32 ConnectionId;
};
typedef struct _FDSAPI_LOGOFF_USER_RESPONSE FDSAPI_LOGOFF_USER_RESPONSE;

struct _FDSAPI_DISCONNECT_USER_REQUEST
{
	FDSAPI_MSG_COMMON();
	UINT32 ConnectionId;
};
typedef struct _FDSAPI_DISCONNECT_USER_REQUEST FDSAPI_DISCONNECT_USER_REQUEST;

struct _FDSAPI_DISCONNECT_USER_RESPONSE
{
	FDSAPI_MSG_COMMON();
	UINT32 ConnectionId;
};
typedef struct _FDSAPI_DISCONNECT_USER_RESPONSE FDSAPI_DISCONNECT_USER_RESPONSE;

struct _FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST
{
	FDSAPI_MSG_COMMON();
	UINT32 ConnectionId;
	char* ServiceEndpoint;
};
typedef struct _FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST;

struct _FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE
{
	FDSAPI_MSG_COMMON();
};
typedef struct _FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE;

struct _FDSAPI_START_SESSION_REQUEST
{
	FDSAPI_MSG_COMMON();
	UINT32 SessionId;
	char* User;
	char* Domain;
	char* Password;
};
typedef struct _FDSAPI_START_SESSION_REQUEST FDSAPI_START_SESSION_REQUEST;

struct _FDSAPI_START_SESSION_RESPONSE
{
	FDSAPI_MSG_COMMON();
	char* ServiceEndpoint;
};
typedef struct _FDSAPI_START_SESSION_RESPONSE FDSAPI_START_SESSION_RESPONSE;

struct _FDSAPI_END_SESSION_REQUEST
{
	FDSAPI_MSG_COMMON();
	UINT32 SessionId;
};
typedef struct _FDSAPI_END_SESSION_REQUEST FDSAPI_END_SESSION_REQUEST;

struct _FDSAPI_END_SESSION_RESPONSE
{
	FDSAPI_MSG_COMMON();
	UINT32 SessionId;
};
typedef struct _FDSAPI_END_SESSION_RESPONSE FDSAPI_END_SESSION_RESPONSE;

struct _FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST
{
	FDSAPI_MSG_COMMON();
	UINT32 ConnectionId;
	char* ChannelName;
};
typedef struct _FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST;

struct _FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE
{
	FDSAPI_MSG_COMMON();
	UINT32 ChannelPort;
	char* ChannelGuid;
};
typedef struct _FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rds_rpc_server rdsRpcServer;
typedef struct rds_rpc_client rdsRpcClient;

typedef int (*pRdsRpcConnectionAccepted)(rdsRpcClient* rpcClient);
typedef int (*pRdsRpcConnectionClosed)(rdsRpcClient* rpcClient);
typedef int (*pRdsRpcMessageReceived)(rdsRpcClient* rpcClient, BYTE* buffer, UINT32 length);

struct rds_rpc_server
{
	void* custom;
	char* Endpoint;
	char* PipeName;
	HANDLE hStopEvent;
	HANDLE hServerThread;

	wArrayList* ClientList;

	pRdsRpcConnectionAccepted ConnectionAccepted;
	pRdsRpcConnectionClosed ConnectionClosed;
	pRdsRpcMessageReceived MessageReceived;
};

struct rds_rpc_client
{
	void* custom;
	char* Endpoint;
	char* PipeName;
	HANDLE hStopEvent;
	HANDLE hClientThread;
	HANDLE hClientPipe;

	rdsRpcServer* RpcServer;

	wStream* InboundStream;

	pRdsRpcConnectionClosed ConnectionClosed;
	pRdsRpcMessageReceived MessageReceived;
};

FREERDS_EXPORT rdsRpcServer* freerds_rpc_server_new(const char* Endpoint);
FREERDS_EXPORT void freerds_rpc_server_free(rdsRpcServer* rpcServer);

FREERDS_EXPORT int freerds_rpc_server_start(rdsRpcServer* rpcServer);
FREERDS_EXPORT int freerds_rpc_server_stop(rdsRpcServer* rpcServer);
FREERDS_EXPORT int freerds_rpc_server_broadcast_message(rdsRpcServer* rpcServer, BYTE* buffer, UINT32 length);

FREERDS_EXPORT rdsRpcClient* freerds_rpc_client_new(const char* Endpoint);
FREERDS_EXPORT void freerds_rpc_client_free(rdsRpcClient* rpcClient);

FREERDS_EXPORT int freerds_rpc_client_start(rdsRpcClient* rpcClient);
FREERDS_EXPORT int freerds_rpc_client_stop(rdsRpcClient* rpcClient);
FREERDS_EXPORT int freerds_rpc_client_send_message(rdsRpcClient* rpcClient, BYTE* buffer, UINT32 length);

/* RPC message packing */

FREERDS_EXPORT UINT32 freerds_rpc_msg_size(UINT32 type);
FREERDS_EXPORT wStream* freerds_rpc_msg_pack(UINT32 type, void* data, wStream* s);
FREERDS_EXPORT BOOL freerds_rpc_msg_unpack(UINT32 type, void* data, const BYTE* buffer, UINT32 size);
FREERDS_EXPORT void freerds_rpc_msg_free(UINT32 type, void* data);

#ifdef __cplusplus
}
#endif

#endif /* FREERDS_RPC_H */

