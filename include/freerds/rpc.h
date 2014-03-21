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

typedef struct rds_rpc rdsRpc;

typedef int (*pRdsRpcAccept)(rdsRpc* rpc);
typedef int (*pRdsRpcReceiveMessage)(rdsRpc* rpc, BYTE* buffer, UINT32 length);

struct rds_rpc
{
	void* custom;
	char* Endpoint;
	char* PipeName;
	BOOL ServerMode;
	HANDLE StopEvent;
	HANDLE ClientThread;
	HANDLE ServerThread;
	HANDLE hClientPipe;
	HANDLE hServerPipe;
	wStream* OutboundStream;
	wStream* InboundStream;
	UINT32 InboundTotalLength;
	UINT32 InboundTotalCount;
	UINT32 OutboundTotalLength;
	UINT32 OutboundTotalCount;

	pRdsRpcAccept Accept;
	pRdsRpcReceiveMessage ReceiveMessage;
};

int freerds_rpc_client_start(rdsRpc* rpc);
int freerds_rpc_client_stop(rdsRpc* rpc);
int freerds_rpc_client_write_message(rdsRpc* rpc, BYTE* buffer, UINT32 length);

rdsRpc* freerds_rpc_client_new(const char* Endpoint);
void freerds_rpc_client_free(rdsRpc* rpc);

int freerds_rpc_server_start(rdsRpc* rpc);
int freerds_rpc_server_stop(rdsRpc* rpc);
int freerds_rpc_server_write_message(rdsRpc* rpc, BYTE* buffer, UINT32 length);

rdsRpc* freerds_rpc_server_new(const char* Endpoint);
void freerds_rpc_server_free(rdsRpc* rpc);

#ifdef __cplusplus
}
#endif

#endif /* FREERDS_RPC_H */

