/**
 * pbRPC - a simple, protocol buffer based RCP mechanism
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
#include <winpr/memory.h>
#include <winpr/thread.h>
#include <stdio.h>

#ifndef WIN32
#include <arpa/inet.h>
#else //WIN32
#include <Winsock2.h>
#endif //WIN32

#include "pbrpc.h"
#include "pbrpc_transport.h"
#include "pbrpc_utils.h"
#include "pbRPC.pb-c.h"

struct pbrpc_transaction
{
	HANDLE Event;
	Freerds__Pbrpc__RPCBase *response;
	UINT32 errorReason;
};
typedef struct pbrpc_transaction pbRPCTransaction;

pbRPCContext *pbrpc_server_new(pbRPCTransportContext *transport)
{
	pbRPCContext *context = malloc(sizeof(pbRPCContext));
	ZeroMemory(context, sizeof(pbRPCContext));
	context->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	context->transport = transport;
	context->transactions = ListDictionary_New(TRUE);
	context->writeQueue = Queue_New(TRUE, -1, -1);
	return context;
}

void pbrpc_server_free(pbRPCContext *context)
{
	if (!context)
		return;
	CloseHandle(context->stopEvent);
	CloseHandle(context->thread);
	ListDictionary_Free(context->transactions);
	Queue_Free(context->writeQueue);
	free(context);
}

// errors < 0 transport erros, errors > 0 pb errors
int pbrpc_receive_message(pbRPCContext *context, char **msg, int *msgLen)
{
  UINT32  msgLenWire, len;
  char *recvbuffer;
  int ret = 0;

  ret = context->transport->read(context->transport, (char *)&msgLenWire, 4);
  if (ret < 0)
	  return ret;

  len = ntohl(msgLenWire);
  *msgLen = len;
  recvbuffer = malloc(len);
  ret = context->transport->read(context->transport, recvbuffer, len);
  if (ret < 0)
  {
	  free(recvbuffer);
	  return ret;
  }
  *msg = recvbuffer;
  return ret;
}

int pbrpc_send_message(pbRPCContext *context, char *msg, UINT32 msgLen)
{
	UINT32 msgLenWire;
	int ret;

	msgLenWire = htonl(msgLen);
	ret = context->transport->write(context->transport, (char *)&msgLenWire, 4);
	if (ret < 0)
		return ret;
	ret = context->transport->write(context->transport, msg, msgLen);
	if (ret < 0)
		return ret;
	return 0;
}

static int pbrpc_process_response(pbRPCContext *context, Freerds__Pbrpc__RPCBase *rpcmessage)
{
	pbRPCTransaction *ta = ListDictionary_GetItemValue(context->transactions, (void *)((UINT_PTR)rpcmessage->tag));
	if (!ta)
	{
		fprintf(stderr,"unsoliciated response - ignoring (tag %d)\n", rpcmessage->tag);
		freerds__pbrpc__rpcbase__free_unpacked(rpcmessage, NULL);
		return 1;
	}
	ListDictionary_Remove(context->transactions, (void *)((UINT_PTR)rpcmessage->tag));
	ta->response = rpcmessage;
	SetEvent(ta->Event);
	return 0;
}

int pbrpc_process_message_out(pbRPCContext *context, Freerds__Pbrpc__RPCBase *msg)
{
	char msgLen = freerds__pbrpc__rpcbase__get_packed_size(msg);
	char *buf = malloc(msgLen);
	int ret;
	ret = freerds__pbrpc__rpcbase__pack(msg, (uint8_t *)buf);
	// packing failed..
	if (ret != msgLen)
	{
		free(buf);
		return 1;
	}
	ret = pbrpc_send_message(context, buf, msgLen);
	free(buf);
	return ret;
}

pbRPCCallback pbrpc_callback_find(pbRPCContext *context, UINT32 type)
{
	pbRPCMethod *cb = NULL;
	int i = 0;
	if(!context->methods)
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
	pbRPCPayload *pl = pbrpc_payload_new();
	pl->data = (char *)(message->payload.data);
	pl->dataLen = message->payload.len;
	pl->errorDescription = message->errordescription;
	return pl;
}

static int pbrpc_process_request(pbRPCContext *context, Freerds__Pbrpc__RPCBase *rpcmessage)
{
	int ret = 0;
	pbRPCCallback cb;
	pbRPCPayload *request = NULL;
	pbRPCPayload *response = NULL;
	Freerds__Pbrpc__RPCBase *pbresponse = pbrpc_message_new();
	pbrpc_prepare_response(pbresponse, rpcmessage->tag);
	pbresponse->msgtype = rpcmessage->msgtype;
	cb = pbrpc_callback_find(context, rpcmessage->msgtype);
	if (NULL == cb)
	{
		pbresponse->status = FREERDS__PBRPC__RPCBASE__RPCSTATUS__NOTFOUND;
		ret = pbrpc_process_message_out(context, pbresponse);
		pbrpc_message_free(pbresponse, TRUE);
		freerds__pbrpc__rpcbase__free_unpacked(rpcmessage, NULL);
		return ret;
	}
	request = pbrpc_fill_payload(rpcmessage);
	ret = cb(request, &response);
	free(request);
	freerds__pbrpc__rpcbase__free_unpacked(rpcmessage, NULL);
	pbresponse->status = ret;
	if (ret == 0)
	{
		pbresponse->has_payload = 1;
		pbresponse->payload.data = (unsigned char *)response->data;
		pbresponse->payload.len = response->dataLen;
	}
	else
	{
		pbresponse->errordescription = response->errorDescription;
	}
	ret = pbrpc_process_message_out(context, pbresponse);
	pbrpc_free_payload(response);
	pbrpc_message_free(pbresponse, FALSE);
	return ret;
}

int pbrpc_process_message_in(pbRPCContext *context)
{
	char *msg;
	int msgLen;
	int ret = 0;
	Freerds__Pbrpc__RPCBase *rpcmessage;
	if (pbrpc_receive_message(context, &msg, &msgLen) < 0)
		return -1;

	rpcmessage = freerds__pbrpc__rpcbase__unpack(NULL, msgLen, (uint8_t *)msg);
	free(msg);
	if (rpcmessage == NULL)
		return 1;

	if(rpcmessage->isresponse)
		ret = pbrpc_process_response(context, rpcmessage);
	else
		ret = pbrpc_process_request(context, rpcmessage);
	return ret;
}

static int pbrpc_transport_open(pbRPCContext *context)
{
	UINT32 sleepInterval = 200;
	while (1)
	{
		if (0 == context->transport->open(context->transport, sleepInterval))
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

static void pbrpc_reconnect(pbRPCContext *context)
{
	context->isConnected = FALSE;
	context->transport->close(context->transport);
	Queue_Clear(context->writeQueue);
	if (0 != pbrpc_transport_open(context))
		return;
	context->isConnected = TRUE;
}

static void pbrpc_mainloop(pbRPCContext *context)
{
	int status;
	DWORD nCount;
	HANDLE events[32];
	while (1)
	{
		nCount = 0;
		HANDLE thandle = context->transport->get_fds(context->transport);
		events[nCount++] = context->stopEvent;
		events[nCount++] = Queue_Event(context->writeQueue);
		events[nCount++] = thandle;
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);
		if (status == WAIT_FAILED)
		{
			continue;
		}
		if (WaitForSingleObject(context->stopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(thandle, 0) == WAIT_OBJECT_0)
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
			Freerds__Pbrpc__RPCBase *msg = NULL;
			while((msg = Queue_Dequeue(context->writeQueue)))
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

int pbrpc_server_start(pbRPCContext *context)
{
	if (pbrpc_transport_open(context) < 0)
		return -1;
	context->isConnected = TRUE;
	context->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) pbrpc_mainloop, context, 0, NULL);
	return 0;
}

int pbrpc_server_stop(pbRPCContext *context)
{
	context->isConnected = FALSE;
	SetEvent(context->stopEvent);
	WaitForSingleObject(context->thread, INFINITE);
	context->transport->close(context->transport);
	return 0;
}

pbRPCTransaction *pbrpc_transaction_new()
{
	pbRPCTransaction *ta = malloc(sizeof(pbRPCTransaction));
	ZeroMemory(ta, sizeof(pbRPCTransaction));
	return ta;
}

void pbrpc_transaction_free(pbRPCTransaction* ta)
{
	if (!ta)
		return;
	free(ta);
}


int pbrpc_call_method(pbRPCContext *context, UINT32 type, pbRPCPayload *request, pbRPCPayload **response)
{
	Freerds__Pbrpc__RPCBase *message;
	pbRPCTransaction *ta = NULL;
	UINT32 ret = PBRPC_FAILED;
	UINT32 tag;
	DWORD wait_ret;
	if (!context->isConnected)
	{
		return PBRCP_TRANSPORT_ERROR;
	}
	message = pbrpc_message_new();
	pbrpc_prepare_request(context, message);
	message->payload.data = (unsigned char*)request->data;
	message->payload.len = request->dataLen;
	message->has_payload = 1;
	message->msgtype = type;
	tag = message->tag;

	ta = pbrpc_transaction_new();
	ta->Event = CreateEvent(NULL, TRUE, FALSE, NULL);
	ListDictionary_Add(context->transactions, (void *)((UINT_PTR)(message->tag)), ta);
	Queue_Enqueue(context->writeQueue, message);

	wait_ret = WaitForSingleObject(ta->Event,PBRPC_TIMEOUT);
	if (wait_ret != WAIT_OBJECT_0)
	{
		ListDictionary_Remove(context->transactions, (void *)((UINT_PTR)(tag)));
		CloseHandle(ta->Event);
		pbrpc_transaction_free(ta);
		return PBRCP_CALL_TIMEOUT;
	}

	CloseHandle(ta->Event);
	message = ta->response;
	if (!message)
	{
		if (ta->errorReason)

			ret = ta->errorReason;
		else
			ret = PBRPC_FAILED;
		pbrpc_transaction_free(ta);
		return ret;
	}
	*response = pbrpc_fill_payload(message);
	ret = message->status;
	pbrpc_transaction_free(ta);
	pbrpc_message_free(message, FALSE);
	return ret;
}

void pbrpc_register_methods(pbRPCContext *context, pbRPCMethod *methods)
{
	context->methods = methods;
}
