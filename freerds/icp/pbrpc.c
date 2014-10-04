/**
 * pbRPC - a simple, protocol buffer based RCP mechanism
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bernhard.miklautz@thincast.com>
 * Copyright 2013 Hardening <contact@hardening-consulting.com>
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

#include <winpr/crt.h>
#include <winpr/pipe.h>
#include <winpr/thread.h>
#include <winpr/interlocked.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#include "pbRPC.pb-c.h"
#include "pbrpc.h"

int tp_npipe_open(pbRPCContext* context, int timeout)
{
	HANDLE hNamedPipe = 0;
	char pipeName[] = "\\\\.\\pipe\\FreeRDS_SessionManager";

	if (!WaitNamedPipeA(pipeName, timeout))
	{
		return -1;
	}

	hNamedPipe = CreateFileA(pipeName,
			GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE))
	{
		return -1;
	}

	context->hPipe = hNamedPipe;

	return 0;
}

int tp_npipe_close(pbRPCContext* context)
{
	if (context->hPipe)
	{
		CloseHandle(context->hPipe);
		context->hPipe = 0;
	}

	return 0;
}

int tp_npipe_write(pbRPCContext* context, char* data, unsigned int datalen)
{
	DWORD bytesWritten;
	BOOL fSuccess = FALSE;

	fSuccess = WriteFile(context->hPipe, data, datalen, (LPDWORD) &bytesWritten, NULL);

	if (!fSuccess || (bytesWritten < datalen))
	{
		return -1;
	}

	return bytesWritten;
}

int tp_npipe_read(pbRPCContext* context, char* data, unsigned int datalen)
{
	DWORD bytesRead;
	BOOL fSuccess = FALSE;

	fSuccess = ReadFile(context->hPipe, data, datalen, &bytesRead, NULL);

	if (!fSuccess || (bytesRead < datalen))
	{
		return -1;
	}

	return bytesRead;
}

struct pbrpc_transaction
{
	pbRpcResponseCallback responseCallback;
	void *callbackArg;
	BOOL freeAfterResponse;
};
typedef struct pbrpc_transaction pbRPCTransaction;

DWORD pbrpc_getTag(pbRPCContext *context)
{
	return InterlockedIncrement(&(context->tag));
}

Freerds__Pbrpc__RPCBase* pbrpc_message_new()
{
	Freerds__Pbrpc__RPCBase* msg = malloc(sizeof(Freerds__Pbrpc__RPCBase));
	if (!msg)
		return msg;

	freerds__pbrpc__rpcbase__init(msg);
	return msg;
}

void pbrpc_message_free(Freerds__Pbrpc__RPCBase* msg, BOOL freePayload)
{
	if (freePayload && msg->payload.data)
	{
		free(msg->payload.data);
	}

	if (freePayload && msg->errordescription)
	{
		free(msg->errordescription);
	}

	free(msg);
}

void pbrpc_prepare_request(pbRPCContext* context, Freerds__Pbrpc__RPCBase* msg)
{
	msg->tag = pbrpc_getTag(context);
	msg->isresponse = FALSE;
	msg->status = FREERDS__PBRPC__RPCBASE__RPCSTATUS__SUCCESS;
}

void pbrpc_prepare_response(Freerds__Pbrpc__RPCBase* msg, UINT32 tag)
{
	msg->isresponse = TRUE;
	msg->tag = tag;
}

void pbrpc_prepare_error(Freerds__Pbrpc__RPCBase* msg, UINT32 tag, char *error)
{
	pbrpc_prepare_response(msg, tag);
	msg->status = FREERDS__PBRPC__RPCBASE__RPCSTATUS__FAILED;
	msg->errordescription = error;
}

pbRPCPayload* pbrpc_payload_new()
{
	pbRPCPayload* pl = calloc(1, sizeof(pbRPCPayload));
	return pl;
}

void pbrpc_free_payload(pbRPCPayload* response)
{
	if (!response)
		return;

	free(response->data);

	if (response->errorDescription)
		free(response->errorDescription);

	free(response);
}

static pbRPCTransaction* pbrpc_transaction_new()
{
	pbRPCTransaction* ta = calloc(1, sizeof(pbRPCTransaction));
	ta->freeAfterResponse = TRUE;
	return ta;
}

void pbrpc_transaction_free(pbRPCTransaction* ta)
{
	if (ta)
		free(ta);
}

static void queue_item_free(void* obj)
{
	pbrpc_message_free((Freerds__Pbrpc__RPCBase*)obj, FALSE);
}

static void list_dictionary_item_free(void* item)
{
	wListDictionaryItem* di = (wListDictionaryItem*) item;
	pbrpc_transaction_free((pbRPCTransaction*)(di->value));
}

pbRPCContext* pbrpc_server_new()
{
	pbRPCContext* context = calloc(1, sizeof(pbRPCContext));

	context->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	context->transactions = ListDictionary_New(TRUE);
	ListDictionary_ValueObject(context->transactions)->fnObjectFree = list_dictionary_item_free;
	context->writeQueue = Queue_New(TRUE, -1, -1);
	context->writeQueue->object.fnObjectFree = queue_item_free;

	return context;
}

void pbrpc_server_free(pbRPCContext* context)
{
	if (!context)
		return;

	CloseHandle(context->stopEvent);
	CloseHandle(context->thread);
	ListDictionary_Free(context->transactions);
	Queue_Free(context->writeQueue);

	free(context);
}

int pbrpc_receive_message(pbRPCContext* context, char** msg, UINT32* msgLen)
{
	char* recvbuffer;
	int status = 0;
	FDSAPI_MSG_HEADER header;

	status = tp_npipe_read(context, (char*) &header, FDSAPI_MSG_HEADER_SIZE);

	if (status < 0)
		return status;

	*msgLen = header.msgSize;
	recvbuffer = malloc(header.msgSize);

	status = tp_npipe_read(context, recvbuffer, header.msgSize);

	if (status < 0)
	{
		free(recvbuffer);
		return status;
	}

	*msg = recvbuffer;

	return status;
}

int pbrpc_send_message(pbRPCContext* context, char* msg, UINT32 msgLen)
{
	int status;
	FDSAPI_MSG_HEADER header;

	header.msgSize = msgLen;
	header.msgType = 0;

	status = tp_npipe_write(context, (char*) &header, FDSAPI_MSG_HEADER_SIZE);

	if (status < 0)
		return status;

	status = tp_npipe_write(context, msg, msgLen);

	if (status < 0)
		return status;

	return 0;
}

static int pbrpc_process_response(pbRPCContext* context, Freerds__Pbrpc__RPCBase *rpcmessage)
{
	pbRPCTransaction* ta = ListDictionary_GetItemValue(context->transactions, (void*)((UINT_PTR)rpcmessage->tag));

	if (!ta)
	{
		fprintf(stderr,"unsoliciated response - ignoring (tag %d)\n", rpcmessage->tag);
		freerds__pbrpc__rpcbase__free_unpacked(rpcmessage, NULL);
		return 1;
	}

	ListDictionary_Remove(context->transactions, (void*)((UINT_PTR)rpcmessage->tag));

	if (ta->responseCallback)
		ta->responseCallback(rpcmessage->status, rpcmessage, ta->callbackArg);

	if (ta->freeAfterResponse)
		free(ta);

	return 0;
}

static int pbrpc_process_message_out(pbRPCContext* context, Freerds__Pbrpc__RPCBase *msg)
{
	int status;
	char msgLen = freerds__pbrpc__rpcbase__get_packed_size(msg);
	char* buf = malloc(msgLen);

	status = freerds__pbrpc__rpcbase__pack(msg, (uint8_t *)buf);

	if (status != msgLen)
		status = 1;
	else
		status = pbrpc_send_message(context, buf, msgLen);

	free(buf);

	return status;
}

static pbRPCCallback pbrpc_callback_find(pbRPCContext* context, UINT32 type)
{
	int i = 0;
	pbRPCMethod* cb = NULL;

	if (!context->methods)
		return NULL;

	while ((cb = &(context->methods[i++])))
	{
		if ((cb->type == 0) && (cb->cb == NULL))
			return NULL;

		if (cb->type == type)
			return cb->cb;
	}

	return NULL;
}

static pbRPCPayload* pbrpc_fill_payload(Freerds__Pbrpc__RPCBase *message)
{
	pbRPCPayload* pl = pbrpc_payload_new();

	pl->data = (char*)(message->payload.data);
	pl->dataLen = message->payload.len;
	pl->errorDescription = message->errordescription;

	return pl;
}

int pbrpc_send_response(pbRPCContext* context, pbRPCPayload* response, UINT32 status, UINT32 type, UINT32 tag)
{
	int ret;
	Freerds__Pbrpc__RPCBase* pbresponse = pbrpc_message_new();

	pbrpc_prepare_response(pbresponse, tag);
	pbresponse->msgtype = type;
	pbresponse->status = status;

	if (response)
	{
		if (status == 0)
		{
			pbresponse->has_payload = 1;
			pbresponse->payload.data = (BYTE*) response->data;
			pbresponse->payload.len = response->dataLen;
		}
		else
		{
			pbresponse->errordescription = response->errorDescription;
		}
	}

	ret = pbrpc_process_message_out(context, pbresponse);

	if (response)
		pbrpc_free_payload(response);

	pbrpc_message_free(pbresponse, FALSE);

	return ret;
}

static int pbrpc_process_request(pbRPCContext* context, Freerds__Pbrpc__RPCBase *rpcmessage)
{
	int status = 0;
	pbRPCCallback cb;
	pbRPCPayload* request = NULL;
	pbRPCPayload* response = NULL;

	cb = pbrpc_callback_find(context, rpcmessage->msgtype);

	if (!cb)
	{
		fprintf(stderr, "server callback not found %d\n", rpcmessage->msgtype);
		status = pbrpc_send_response(context, NULL, FREERDS__PBRPC__RPCBASE__RPCSTATUS__NOTFOUND, rpcmessage->msgtype, rpcmessage->tag);
		freerds__pbrpc__rpcbase__free_unpacked(rpcmessage, NULL);
		return status;
	}

	request = pbrpc_fill_payload(rpcmessage);
	status = cb(rpcmessage->tag, request, &response);
	free(request);

	/* If callback doesn't set a respond response needs to be sent async */
	if (!response)
	{
		freerds__pbrpc__rpcbase__free_unpacked(rpcmessage, NULL);
		return 0;
	}

	status = pbrpc_send_response(context, response, status, rpcmessage->msgtype, rpcmessage->tag);
	freerds__pbrpc__rpcbase__free_unpacked(rpcmessage, NULL);

	return status;
}

int pbrpc_process_message_in(pbRPCContext* context)
{
	char* msg;
	UINT32 msgLen;
	int status = 0;
	Freerds__Pbrpc__RPCBase* rpcmessage;

	if (pbrpc_receive_message(context, &msg, &msgLen) < 0)
		return -1;

	rpcmessage = freerds__pbrpc__rpcbase__unpack(NULL, msgLen, (uint8_t*) msg);

	free(msg);

	if (!rpcmessage)
		return 1;

	if (rpcmessage->isresponse)
		status = pbrpc_process_response(context, rpcmessage);
	else
		status = pbrpc_process_request(context, rpcmessage);

	return status;
}

static int pbrpc_transport_open(pbRPCContext* context)
{
	UINT32 sleepInterval = 200;

	while (1)
	{
		if (tp_npipe_open(context, sleepInterval) == 0)
		{
			return 0;
		}

		if (WaitForSingleObject(context->stopEvent, 0) == WAIT_OBJECT_0)
		{
			return -1;
		}

		Sleep(sleepInterval);
	}

	return 0;
}

static void pbrpc_connect(pbRPCContext* context)
{
	if (0 != pbrpc_transport_open(context))
		return;

	context->isConnected = TRUE;
}

static void pbrpc_reconnect(pbRPCContext* context)
{
	pbRPCTransaction* ta = NULL;
	context->isConnected = FALSE;

	tp_npipe_close(context);
	Queue_Clear(context->writeQueue);

	while ((ta = ListDictionary_Remove_Head(context->transactions)))
	{
		ta->responseCallback(PBRCP_TRANSPORT_ERROR, 0, ta->callbackArg);

		if (ta->freeAfterResponse)
			free(ta);
	}

	pbrpc_connect(context);
}

static void pbrpc_mainloop(pbRPCContext* context)
{
	int status;
	DWORD nCount;
	HANDLE events[32];

	pbrpc_connect(context);

	while (context->isConnected)
	{
		nCount = 0;
		events[nCount++] = context->stopEvent;
		events[nCount++] = Queue_Event(context->writeQueue);
		events[nCount++] = context->hPipe;

		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (status == WAIT_FAILED)
		{
			break;
		}

		if (WaitForSingleObject(context->stopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(context->hPipe, 0) == WAIT_OBJECT_0)
		{
			status = pbrpc_process_message_in(context);

			if (status < 0)
			{
				fprintf(stderr, "Transport problem reconnecting..\n");
				pbrpc_reconnect(context);
				continue;
			}
		}

		if (WaitForSingleObject(Queue_Event(context->writeQueue), 0) == WAIT_OBJECT_0)
		{
			Freerds__Pbrpc__RPCBase* msg = NULL;

			while ((msg = Queue_Dequeue(context->writeQueue)))
			{
				status = pbrpc_process_message_out(context, msg);
				pbrpc_message_free(msg, FALSE);
			}

			if (status < 0)
			{
				fprintf(stderr, "Transport problem reconnecting..\n");
				pbrpc_reconnect(context);
				continue;
			}
		}
	}
}

int pbrpc_server_start(pbRPCContext* context)
{
	context->isConnected = FALSE;
	context->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) pbrpc_mainloop, context, 0, NULL);
	return 0;
}

int pbrpc_server_stop(pbRPCContext* context)
{
	context->isConnected = FALSE;
	SetEvent(context->stopEvent);
	WaitForSingleObject(context->thread, INFINITE);
	tp_npipe_close(context);
	return 0;
}

/** @brief contextual data to handle a local call */
struct pbrpc_local_call_context {
	HANDLE event;
	Freerds__Pbrpc__RPCBase *response;
	PBRPCSTATUS status;
};

static void pbrpc_response_local_cb(PBRPCSTATUS reason, Freerds__Pbrpc__RPCBase* response, void *args)
{
	struct pbrpc_local_call_context *context = (struct pbrpc_local_call_context *)args;
	context->response = response;
	context->status = reason;
	SetEvent(context->event);
}

int pbrpc_call_method(pbRPCContext* context, UINT32 type, pbRPCPayload* request, pbRPCPayload** response)
{
	Freerds__Pbrpc__RPCBase* message;
	pbRPCTransaction ta;
	UINT32 ret = PBRPC_FAILED;
	UINT32 tag;
	DWORD wait_ret;
	struct pbrpc_local_call_context local_context;

	if (!context->isConnected)
	{
		return PBRCP_TRANSPORT_ERROR;
	}

	message = pbrpc_message_new();
	pbrpc_prepare_request(context, message);
	message->payload.data = (BYTE*) request->data;
	message->payload.len = request->dataLen;
	message->has_payload = 1;
	message->msgtype = type;
	tag = message->tag;

	ZeroMemory(&local_context, sizeof(local_context));
	local_context.event = CreateEvent(NULL, TRUE, FALSE, NULL);
	local_context.status = PBRCP_CALL_TIMEOUT;

	ta.responseCallback = pbrpc_response_local_cb;
	ta.callbackArg = &local_context;
	ta.freeAfterResponse = FALSE;

	// TODO: the funky case can occur because an equivalent of pthread_condition is
	//		not used, unfortunately this kind of primitive does not exists under win32
	//      nor with winpr
	// 	the clean case should be:
	//
	//  Lock(context->transactions)
	//    ....
	//  pthread_cond_wait(local_context.event, context->transactions.lock)
	//    ...
	// 	Unlock(context->transactions)

	ListDictionary_Add(context->transactions, (void*)((UINT_PTR)(message->tag)), &ta);
	Queue_Enqueue(context->writeQueue, message);

	wait_ret = WaitForSingleObject(local_context.event, PBRPC_TIMEOUT);

	if (wait_ret != WAIT_OBJECT_0)
	{
		if(!ListDictionary_Remove(context->transactions, (void*)((UINT_PTR)(tag))))
		{
			// special case - timeout occurred but request is already processing, see comment above
			WaitForSingleObject(local_context.event, INFINITE);
		}
		else
		{
			ret = PBRCP_CALL_TIMEOUT;
		}
	}

	CloseHandle(local_context.event);
	message = local_context.response;

	if (!message)
	{
		if (local_context.status)
			ret = local_context.status;
		else
			ret = PBRPC_FAILED;
	}
	else
	{
		*response = pbrpc_fill_payload(message);
		ret = message->status;
		pbrpc_message_free(message, FALSE);
	}

	return ret;
}

void pbrpc_register_methods(pbRPCContext* context, pbRPCMethod *methods)
{
	context->methods = methods;
}

void pbrcp_call_method_async(pbRPCContext* context, UINT32 type, pbRPCPayload* request,
		pbRpcResponseCallback callback, void *callback_args)
{
	Freerds__Pbrpc__RPCBase* message;

	if (!context->isConnected)
	{
		callback(PBRCP_TRANSPORT_ERROR, 0, callback_args);
		return;
	}

	pbRPCTransaction *ta = pbrpc_transaction_new();
	ta->responseCallback = callback;
	ta->callbackArg = callback_args;

	message = pbrpc_message_new();
	pbrpc_prepare_request(context, message);
	message->payload.data = (unsigned char*)request->data;
	message->payload.len = request->dataLen;
	message->has_payload = 1;
	message->msgtype = type;

	ListDictionary_Add(context->transactions, (void*)((UINT_PTR)(message->tag)), ta);
	Queue_Enqueue(context->writeQueue, message);
}
