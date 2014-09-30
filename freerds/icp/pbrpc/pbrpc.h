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
#include <winpr/wtypes.h>
#include <winpr/collections.h>

#include "pbrpc_transport.h"

#define PBRPC_TIMEOUT 10000

typedef struct pbrpc_method pbRPCMethod;

struct  pbrpc_context
{
	HANDLE stopEvent;
	HANDLE thread;
	pbRPCTransportContext* transport;
	wListDictionary* transactions;
	wQueue* writeQueue;
	BOOL isConnected;
	LONG tag;
	pbRPCMethod* methods;
};
typedef struct pbrpc_context pbRPCContext;

struct pbrpc_payload
{
	char* data;
	UINT32 dataLen;
	char* errorDescription;
};
typedef struct pbrpc_payload pbRPCPayload;

typedef int (*pbRPCCallback)(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse);

struct pbrpc_method
{
	UINT32 type;
	pbRPCCallback cb;
};

/* Return codes 0-100 are used from the pbrpc protocol itself */
typedef enum pbrpc_status
{
	PBRPC_SUCCESS = 0, // everything is fine
	PBRPC_FAILED = 1, // request failed optional error string might be set
	PBRPC_NOTFOUND = 2, // method was not found on server
	PBRPC_BAD_REQEST_DATA = 100, // request couldn't be serialized
	PBRPC_BAD_RESPONSE = 101, // response couldn't be  unserialized
	PBRCP_TRANSPORT_ERROR = 102, // problem with transport
	PBRCP_CALL_TIMEOUT = 103 // problem with transport
} PBRPCSTATUS;


#ifndef PROTOBUF_C_pbRPC_2eproto__INCLUDED
typedef struct _Freerds__Pbrpc__RPCBase Freerds__Pbrpc__RPCBase;
#endif
typedef void (*pbRpcResponseCallback)(UINT32 reason, Freerds__Pbrpc__RPCBase* response, void *args);

pbRPCContext* pbrpc_server_new(pbRPCTransportContext* transport);
void pbrpc_server_free(pbRPCContext* context);
int pbrpc_server_start(pbRPCContext* context);
int pbrpc_server_stop(pbRPCContext* context);
int pbrpc_call_method(pbRPCContext* context, UINT32 type, pbRPCPayload* request, pbRPCPayload** response);
void pbrcp_call_method_async(pbRPCContext* context, UINT32 type, pbRPCPayload* request,
		pbRpcResponseCallback callback, void *callback_args);
void pbrpc_register_methods(pbRPCContext* context, pbRPCMethod* methods);
int pbrpc_send_response(pbRPCContext* context, pbRPCPayload *response, UINT32 status, UINT32 type,
UINT32 tag);


#endif //_PBRPC_H
