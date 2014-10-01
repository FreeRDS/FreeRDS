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

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/wtypes.h>
#include <winpr/stream.h>
#include <winpr/collections.h>

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

rdsRpcServer* freerds_rpc_server_new(const char* Endpoint);
void freerds_rpc_server_free(rdsRpcServer* rpcServer);

int freerds_rpc_server_start(rdsRpcServer* rpcServer);
int freerds_rpc_server_stop(rdsRpcServer* rpcServer);
int freerds_rpc_server_broadcast_message(rdsRpcServer* rpcServer, BYTE* buffer, UINT32 length);

rdsRpcClient* freerds_rpc_client_new(const char* Endpoint);
void freerds_rpc_client_free(rdsRpcClient* rpcClient);

int freerds_rpc_client_start(rdsRpcClient* rpcClient);
int freerds_rpc_client_stop(rdsRpcClient* rpcClient);
int freerds_rpc_client_send_message(rdsRpcClient* rpcClient, BYTE* buffer, UINT32 length);

#ifdef __cplusplus
}
#endif

#endif /* FREERDS_RPC_H */

