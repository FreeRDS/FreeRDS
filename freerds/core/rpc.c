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

#include "core.h"
#include "app_context.h"

#include "rpc.h"

static pbRPCContext* g_pbContext = NULL;

int freerds_icp_start()
{
	if (!g_pbContext)
	{
		g_pbContext = pbrpc_server_new();
		pbrpc_server_start(g_pbContext);
	}

	return 0;
}

int freerds_icp_shutdown()
{
	if (g_pbContext)
	{
		pbrpc_server_stop(g_pbContext);
		pbrpc_server_free(g_pbContext);
		g_pbContext = NULL;
	}

	return 0;
}

void* freerds_icp_get_context()
{
	return (void*) g_pbContext;
}

int tp_npipe_open(pbRPCContext* context, int timeout)
{
	HANDLE hNamedPipe = 0;
	char pipeName[] = "\\\\.\\pipe\\FreeRDS_Manager";

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

int tp_npipe_write(pbRPCContext* context, BYTE* data, UINT32 size)
{
	DWORD bytesWritten;
	BOOL fSuccess = FALSE;

	fSuccess = WriteFile(context->hPipe, data, size, (LPDWORD) &bytesWritten, NULL);

	if (!fSuccess || (bytesWritten < size))
	{
		return -1;
	}

	return bytesWritten;
}

int tp_npipe_read(pbRPCContext* context, BYTE* data, UINT32 size)
{
	DWORD bytesRead;
	BOOL fSuccess = FALSE;

	fSuccess = ReadFile(context->hPipe, data, size, &bytesRead, NULL);

	if (!fSuccess || (bytesRead < size))
	{
		return -1;
	}

	return bytesRead;
}

struct pbrpc_transaction
{
	pbRpcResponseCallback responseCallback;
	void* callbackArg;
	BOOL freeAfterResponse;
};
typedef struct pbrpc_transaction pbRPCTransaction;

DWORD pbrpc_getTag(pbRPCContext *context)
{
	return InterlockedIncrement(&(context->tag));
}

FDSAPI_MSG_PACKET* pbrpc_message_new()
{
	FDSAPI_MSG_PACKET* msg = calloc(1, sizeof(FDSAPI_MSG_PACKET));

	if (!msg)
		return msg;

	return msg;
}

void pbrpc_message_free(FDSAPI_MSG_PACKET* msg, BOOL freePayload)
{
	if (freePayload)
		free(msg->buffer);

	free(msg);
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

	free(response->buffer);
	free(response);
}

static void queue_item_free(void* obj)
{
	pbrpc_message_free((FDSAPI_MSG_PACKET*)obj, FALSE);
}

static void list_dictionary_item_free(void* item)
{
	wListDictionaryItem* di = (wListDictionaryItem*) item;
	free((pbRPCTransaction*)(di->value));
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

int pbrpc_receive_message(pbRPCContext* context, FDSAPI_MSG_HEADER* header, BYTE** buffer)
{
	int status = 0;

	status = tp_npipe_read(context, (BYTE*) header, FDSAPI_MSG_HEADER_SIZE);

	if (status < 0)
		return status;

	*buffer = malloc(header->msgSize);

	if (!(*buffer))
		return -1;

	status = tp_npipe_read(context, *buffer, header->msgSize);

	if (status < 0)
	{
		free(*buffer);
		return status;
	}

	return status;
}

int pbrpc_send_message(pbRPCContext* context, FDSAPI_MSG_HEADER* header, BYTE* buffer)
{
	int status;

	status = tp_npipe_write(context, (BYTE*) header, FDSAPI_MSG_HEADER_SIZE);

	if (status < 0)
		return status;

	status = tp_npipe_write(context, buffer, header->msgSize);

	if (status < 0)
		return status;

	return 0;
}

static int pbrpc_process_response(pbRPCContext* context, FDSAPI_MSG_PACKET* msg)
{
	pbRPCTransaction* ta = ListDictionary_GetItemValue(context->transactions, (void*)((UINT_PTR)msg->callId));

	if (!ta)
	{
		fprintf(stderr,"unsoliciated response - ignoring (tag %d)\n", msg->callId);
		return 1;
	}

	ListDictionary_Remove(context->transactions, (void*)((UINT_PTR)msg->callId));

	if (ta->responseCallback)
		ta->responseCallback(msg->status, msg, ta->callbackArg);

	if (ta->freeAfterResponse)
		free(ta);

	return 0;
}

static int pbrpc_process_message_out(pbRPCContext* context, FDSAPI_MSG_PACKET* msg)
{
	int status;
	FDSAPI_MSG_HEADER header;

	header.msgType = msg->msgType;
	header.msgSize = msg->length;
	header.callId = msg->callId;
	header.status = msg->status;

	status = pbrpc_send_message(context, &header, msg->buffer);

	return status;
}

static pbRPCPayload* pbrpc_fill_payload(FDSAPI_MSG_PACKET* msg)
{
	pbRPCPayload* pl = (pbRPCPayload*) calloc(1, sizeof(pbRPCPayload));

	pl->buffer = msg->buffer;
	pl->length = msg->length;

	return pl;
}

int pbrpc_send_response(pbRPCContext* context, pbRPCPayload* response, UINT32 status, UINT32 type, UINT32 tag)
{
	int ret;
	FDSAPI_MSG_PACKET* msg;

	msg = pbrpc_message_new();

	msg->callId = tag;
	msg->msgType = FDSAPI_RESPONSE_ID(type);
	msg->status = status;

	if (response)
	{
		if (status == 0)
		{
			msg->buffer = response->buffer;
			msg->length = response->length;
		}
	}

	ret = pbrpc_process_message_out(context, msg);

	if (response)
	{
		free(response->buffer);
		free(response);
	}

	pbrpc_message_free(msg, FALSE);

	return ret;
}

static int pbrpc_process_request(pbRPCContext* context, FDSAPI_MSG_PACKET* msg)
{
	int status = 0;
	UINT32 msgType;
	pbRPCCallback cb;
	pbRPCPayload* request = NULL;
	pbRPCPayload* response = NULL;

	msgType = msg->msgType;

	switch (msgType)
	{
		case FDSAPI_HEARTBEAT_REQUEST_ID:
			cb = freerds_icp_Heartbeat;
			break;

		case FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID:
			cb = freerds_icp_SwitchServiceEndpoint;
			break;

		case FDSAPI_LOGOFF_USER_REQUEST_ID:
			cb = freerds_icp_LogoffUser;
			break;

		case FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST_ID:
			cb = freerds_icp_ChannelEndpointOpen;
			break;
	}

	if (!cb)
	{
		fprintf(stderr, "server callback not found %d\n", msg->msgType);
		msg->status = FDSAPI_STATUS_NOTFOUND;
		status = pbrpc_send_response(context, NULL, msg->status,
				msg->msgType, msg->callId);
		return status;
	}

	request = pbrpc_fill_payload(msg);
	status = cb(msg->callId, request, &response);
	free(request);

	if (!response)
		return 0;

	status = pbrpc_send_response(context, response, status, msg->msgType, msg->callId);

	return status;
}

int pbrpc_process_message_in(pbRPCContext* context)
{
	BYTE* buffer;
	int status = 0;
	FDSAPI_MSG_HEADER header;
	FDSAPI_MSG_PACKET* msg;

	if (pbrpc_receive_message(context, &header, &buffer) < 0)
		return -1;

	msg = (FDSAPI_MSG_PACKET*) calloc(1, sizeof(FDSAPI_MSG_PACKET));

	CopyMemory(msg, &header, sizeof(FDSAPI_MSG_HEADER));

	msg->buffer = buffer;
	msg->length = header.msgSize;

	if (FDSAPI_IS_RESPONSE_ID(msg->msgType))
		status = pbrpc_process_response(context, msg);
	else
		status = pbrpc_process_request(context, msg);

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

static void pbrpc_main_loop(pbRPCContext* context)
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
			FDSAPI_MSG_PACKET* msg = NULL;

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
	context->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) pbrpc_main_loop, context, 0, NULL);
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
	FDSAPI_MSG_PACKET *response;
	PBRPCSTATUS status;
};

static void pbrpc_response_local_cb(PBRPCSTATUS reason, FDSAPI_MSG_PACKET* response, void *args)
{
	struct pbrpc_local_call_context *context = (struct pbrpc_local_call_context*) args;
	context->response = response;
	context->status = reason;
	SetEvent(context->event);
}

int pbrpc_call_method(pbRPCContext* context, UINT32 type, pbRPCPayload* request, pbRPCPayload** response)
{
	UINT32 tag;
	DWORD wait_ret;
	pbRPCTransaction ta;
	UINT32 ret = PBRPC_FAILED;
	FDSAPI_MSG_PACKET* msg;
	struct pbrpc_local_call_context local_context;

	if (!context->isConnected)
		return PBRCP_TRANSPORT_ERROR;

	tag = pbrpc_getTag(context);

	msg = pbrpc_message_new();

	msg->callId = tag;
	msg->status = FDSAPI_STATUS_SUCCESS;
	msg->buffer = request->buffer;
	msg->length = request->length;
	msg->msgType = FDSAPI_REQUEST_ID(type);

	ZeroMemory(&local_context, sizeof(local_context));
	local_context.event = CreateEvent(NULL, TRUE, FALSE, NULL);
	local_context.status = PBRCP_CALL_TIMEOUT;

	ta.responseCallback = pbrpc_response_local_cb;
	ta.callbackArg = &local_context;
	ta.freeAfterResponse = FALSE;

	ListDictionary_Add(context->transactions, (void*)((UINT_PTR)(msg->callId)), &ta);
	Queue_Enqueue(context->writeQueue, msg);

	wait_ret = WaitForSingleObject(local_context.event, PBRPC_TIMEOUT);

	if (wait_ret != WAIT_OBJECT_0)
	{
		if (!ListDictionary_Remove(context->transactions, (void*)((UINT_PTR)(tag))))
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
	msg = local_context.response;

	if (!msg)
	{
		if (local_context.status)
			ret = local_context.status;
		else
			ret = PBRPC_FAILED;
	}
	else
	{
		*response = pbrpc_fill_payload(msg);
		ret = msg->status;
		pbrpc_message_free(msg, FALSE);
	}

	return ret;
}

void pbrcp_call_method_async(pbRPCContext* context, UINT32 type, pbRPCPayload* request,
		pbRpcResponseCallback callback, void *callback_args)
{
	UINT32 tag;
	pbRPCTransaction* ta;
	FDSAPI_MSG_PACKET* msg;

	if (!context->isConnected)
	{
		callback(PBRCP_TRANSPORT_ERROR, 0, callback_args);
		return;
	}

	ta = (pbRPCTransaction*) calloc(1, sizeof(pbRPCTransaction));
	ta->freeAfterResponse = TRUE;
	ta->responseCallback = callback;
	ta->callbackArg = callback_args;

	tag = pbrpc_getTag(context);

	msg = pbrpc_message_new();

	msg->callId = tag;
	msg->status = FDSAPI_STATUS_SUCCESS;
	msg->buffer = request->buffer;
	msg->length = request->length;
	msg->msgType = FDSAPI_REQUEST_ID(type);

	ListDictionary_Add(context->transactions, (void*)((UINT_PTR)(msg->callId)), ta);
	Queue_Enqueue(context->writeQueue, msg);
}

/* RPC Client Stubs */

int freerds_icp_LogoffUserResponse(FDSAPI_LOGOFF_USER_RESPONSE* pResponse)
{
	pbRPCPayload* pbresponse;
	UINT32 type = FDSAPI_LOGOFF_USER_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return -1;

	pResponse->status = 0;
	pResponse->msgType = FDSAPI_LOGOFF_USER_RESPONSE_ID;

	pbresponse = pbrpc_payload_new();
	pbresponse->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), pResponse, NULL);
	pbresponse->buffer = Stream_Buffer(pbresponse->s);
	pbresponse->length = Stream_Length(pbresponse->s);

	return pbrpc_send_response(context, pbresponse, pResponse->status, pResponse->msgType, pResponse->callId);
}

int freerds_icp_SwitchServiceEndpointResponse(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE* pResponse)
{
	pbRPCPayload* pbresponse;
	UINT32 type = FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return -1;

	pResponse->status = 0;
	pResponse->msgType = FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE_ID;

	pbresponse = pbrpc_payload_new();
	pbresponse->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), pResponse, NULL);
	pbresponse->buffer = Stream_Buffer(pbresponse->s);
	pbresponse->length = Stream_Length(pbresponse->s);

	return pbrpc_send_response(context, pbresponse, pResponse->status, pResponse->msgType, pResponse->callId);
}

int freerds_icp_IsChannelAllowed(FDSAPI_CHANNEL_ALLOWED_REQUEST* pRequest, FDSAPI_CHANNEL_ALLOWED_RESPONSE* pResponse)
{
	int status;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	UINT32 type = FDSAPI_CHANNEL_ALLOWED_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	pbrequest.s = freerds_rpc_msg_pack(type, pRequest, NULL);
	pbrequest.buffer = Stream_Buffer(pbrequest.s);
	pbrequest.length = Stream_Length(pbrequest.s);

	status = pbrpc_call_method(context, FDSAPI_REQUEST_ID(type), &pbrequest, &pbresponse);

	Stream_Free(pbrequest.s, TRUE);

	if (status != 0)
		return status;

	freerds_rpc_msg_unpack(FDSAPI_RESPONSE_ID(type), pResponse, pbresponse->buffer, pbresponse->length);

	pbrpc_free_payload(pbresponse);

	return PBRPC_SUCCESS;
}

int freerds_icp_DisconnectUserSession(FDSAPI_DISCONNECT_USER_REQUEST* pRequest, FDSAPI_DISCONNECT_USER_RESPONSE* pResponse)
{
	int status;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	UINT32 type = FDSAPI_DISCONNECT_USER_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	pbrequest.s = freerds_rpc_msg_pack(type, pRequest, NULL);
	pbrequest.buffer = Stream_Buffer(pbrequest.s);
	pbrequest.length = Stream_Length(pbrequest.s);

	status = pbrpc_call_method(context, FDSAPI_REQUEST_ID(type), &pbrequest, &pbresponse);

	Stream_Free(pbrequest.s, TRUE);

	if (status != 0)
		return status;

	freerds_rpc_msg_unpack(FDSAPI_RESPONSE_ID(type), pResponse, pbresponse->buffer, pbresponse->length);

	pbrpc_free_payload(pbresponse);

	return PBRPC_SUCCESS;
}

int freerds_icp_LogOffUserSession(FDSAPI_LOGOFF_USER_REQUEST* pRequest, FDSAPI_LOGOFF_USER_RESPONSE* pResponse)
{
	int status;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	UINT32 type = FDSAPI_LOGOFF_USER_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	pbrequest.s = freerds_rpc_msg_pack(type, pRequest, NULL);
	pbrequest.buffer = Stream_Buffer(pbrequest.s);
	pbrequest.length = Stream_Length(pbrequest.s);

	status = pbrpc_call_method(context, FDSAPI_REQUEST_ID(type), &pbrequest, &pbresponse);

	Stream_Free(pbrequest.s, TRUE);

	if (status != 0)
		return status;

	freerds_rpc_msg_unpack(FDSAPI_RESPONSE_ID(type), pResponse, pbresponse->buffer, pbresponse->length);

	pbrpc_free_payload(pbresponse);

	return PBRPC_SUCCESS;
}

int freerds_icp_LogonUser(FDSAPI_LOGON_USER_REQUEST* pRequest, FDSAPI_LOGON_USER_RESPONSE* pResponse)
{
	int status;
	pbRPCPayload pbrequest;
	pbRPCPayload* pbresponse = NULL;
	UINT32 type = FDSAPI_LOGON_USER_REQUEST_ID;
	pbRPCContext* context = (pbRPCContext*) freerds_icp_get_context();

	if (!context)
		return PBRPC_FAILED;

	pbrequest.s = freerds_rpc_msg_pack(type, pRequest, NULL);
	pbrequest.buffer = Stream_Buffer(pbrequest.s);
	pbrequest.length = Stream_Length(pbrequest.s);

	status = pbrpc_call_method(context, FDSAPI_REQUEST_ID(type), &pbrequest, &pbresponse);

	Stream_Free(pbrequest.s, TRUE);

	if (status != 0)
		return status;

	freerds_rpc_msg_unpack(FDSAPI_RESPONSE_ID(type), pResponse, pbresponse->buffer, pbresponse->length);

	pbrpc_free_payload(pbresponse);

	return PBRPC_SUCCESS;
}

/* RPC server stubs */

int freerds_icp_Heartbeat(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	pbRPCPayload* payload;
	FDSAPI_HEARTBEAT_REQUEST request;
	FDSAPI_HEARTBEAT_RESPONSE response;
	UINT32 type = FDSAPI_HEARTBEAT_REQUEST_ID;

	freerds_rpc_msg_unpack(FDSAPI_REQUEST_ID(type), &request, pbrequest->buffer, pbrequest->length);

	response.msgType = FDSAPI_RESPONSE_ID(type);
	response.callId = request.callId;
	response.HeartbeatId = request.HeartbeatId;

	payload = pbrpc_payload_new();
	payload->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), &response, NULL);
	payload->buffer = Stream_Buffer(payload->s);
	payload->length = Stream_Length(payload->s);
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

int freerds_icp_SwitchServiceEndpoint(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	pbRPCPayload* payload;
	rdsConnection* connection = NULL;
	FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST request;
	FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE response;
	UINT32 type = FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID;

	freerds_rpc_msg_unpack(FDSAPI_REQUEST_ID(type), &request, pbrequest->buffer, pbrequest->length);

	response.msgType = FDSAPI_RESPONSE_ID(type);
	response.callId = request.callId;

	connection = app_context_get_connection(request.ConnectionId);

	if (connection)
	{
		FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST* pRequest;

		pRequest = (FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST*) malloc(sizeof(FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST));

		pRequest->callId = tag;
		pRequest->msgType = FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID;
		pRequest->ServiceEndpoint = _strdup(request.ServiceEndpoint);

		MessageQueue_Post(connection->notifications, (void*) connection, pRequest->msgType, (void*) pRequest, NULL);
		*pbresponse = NULL;
		return 0;
	}
	else
	{
		fprintf(stderr, "something went wrong\n");
		response.status = 1;
	}

	payload = pbrpc_payload_new();
	payload->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), &response, NULL);
	payload->buffer = Stream_Buffer(payload->s);
	payload->length = Stream_Length(payload->s);
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

int freerds_icp_LogoffUser(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	pbRPCPayload* payload;
	rdsConnection* connection = NULL;
	FDSAPI_LOGOFF_USER_REQUEST request;
	FDSAPI_LOGOFF_USER_RESPONSE response;
	UINT32 type = FDSAPI_LOGOFF_USER_REQUEST_ID;

	freerds_rpc_msg_unpack(FDSAPI_REQUEST_ID(type), &request, pbrequest->buffer, pbrequest->length);

	response.msgType = FDSAPI_RESPONSE_ID(type);
	response.callId = request.callId;

	connection = app_context_get_connection(request.ConnectionId);

	if (connection)
	{
		FDSAPI_LOGOFF_USER_REQUEST* pRequest;

		pRequest = (FDSAPI_LOGOFF_USER_REQUEST*) malloc(sizeof(FDSAPI_LOGOFF_USER_REQUEST));

		pRequest->callId = tag;
		pRequest->msgType = FDSAPI_LOGOFF_USER_REQUEST_ID;

		MessageQueue_Post(connection->notifications, (void*) connection, pRequest->msgType, (void*) pRequest, NULL);

		*pbresponse = NULL;
		return 0;
	}
	else
	{
		fprintf(stderr, "something went wrong\n");
		response.status = 1;
	}

	payload = pbrpc_payload_new();
	payload->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), &response, NULL);
	payload->buffer = Stream_Buffer(payload->s);
	payload->length = Stream_Length(payload->s);
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

int freerds_icp_ChannelEndpointOpen(LONG tag, pbRPCPayload* pbrequest, pbRPCPayload** pbresponse)
{
	pbRPCPayload* payload;
	FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST request;
	FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE response;
	UINT32 type = FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST_ID;

	freerds_rpc_msg_unpack(FDSAPI_REQUEST_ID(type), &request, pbrequest->buffer, pbrequest->length);

	response.msgType = FDSAPI_RESPONSE_ID(type);
	response.callId = request.callId;
	response.status = 0;

	payload = pbrpc_payload_new();
	payload->s = freerds_rpc_msg_pack(FDSAPI_RESPONSE_ID(type), &response, NULL);
	payload->buffer = Stream_Buffer(payload->s);
	payload->length = Stream_Length(payload->s);
	*pbresponse = payload;

	return PBRPC_SUCCESS;
}

