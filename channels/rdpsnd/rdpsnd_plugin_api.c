/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server plugin API for RDPSND
 *
 * Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
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
#include <winpr/wlog.h>
#include <winpr/wtypes.h>

#include <freerds/rpc.h>

#include "channel_utils.h"

#include "rdpsnd_plugin_api.h"
#include "rdpsnd_plugin_msgs.h"

typedef struct
{
	wLog *log;

	rdsRpcClient *rpcClient;
	HANDLE hWaitEvent;
	wStream *response;
}
rdpsnd_plugin_api_context;

static int rdpsnd_plugin_api_connection_closed(rdsRpcClient *rpcClient)
{
	rdpsnd_plugin_api_context *context;

	context = (rdpsnd_plugin_api_context *) rpcClient->custom;

	freerds_rpc_client_free(context->rpcClient);

	return 0;
}

static int rdpsnd_plugin_api_message_received(rdsRpcClient *rpcClient, BYTE *buffer, UINT32 length)
{
	rdpsnd_plugin_api_context *context;

	wStream *response;

	context = (rdpsnd_plugin_api_context *) rpcClient->custom;

	response = Stream_New(NULL, length);
	if (response)
	{
		Stream_Write(response, buffer, length);
		Stream_SetPosition(response, 0);

		context->response = response;

		SetEvent(context->hWaitEvent);
	}

	return 0;
}

static BOOL rdpsnd_plugin_api_send_request(rdpsnd_plugin_api_context *context, wStream *request)
{
	BYTE *requestBuffer;
	UINT32 requestLength;
	int status;

	/* Ensure the named pipe has been opened. */
	if (!context->rpcClient)
	{
		WLog_Print(context->log, WLOG_ERROR, "No connection has been made");

		return FALSE;
	}

	/* Clear the wait event. */
	ResetEvent(context->hWaitEvent);

	/* Send the request. */
	requestBuffer = Stream_Buffer(request);
	requestLength = Stream_GetPosition(request);

	status = freerds_rpc_client_send_message(context->rpcClient, Stream_Buffer(request), Stream_GetPosition(request));
	if (status != requestLength)
	{
		WLog_Print(context->log, WLOG_ERROR, "Error sending request (status=%d)", status);

		return FALSE;
	}

	return TRUE;
}

static BOOL rdpsnd_plugin_api_wait_response(rdpsnd_plugin_api_context *context, wStream **response)
{
	/* Wait for a response to be received. */
	WaitForSingleObject(context->hWaitEvent, INFINITE);

	*response = context->response;

	return context->response ? TRUE : FALSE;
}

void *rdpsnd_plugin_api_new()
{
	rdpsnd_plugin_api_context *context;

	context = (rdpsnd_plugin_api_context *) malloc(sizeof(rdpsnd_plugin_api_context));
	if (context)
	{
		ZeroMemory(context, sizeof(rdpsnd_plugin_api_context));

		context->log = WLog_Get("freerds.channels.rdpsnd");

		context->hWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	return context;
}

void rdpsnd_plugin_api_free(void *api_context)
{
	rdpsnd_plugin_api_context *context;

	if (!api_context) return;

	context = (rdpsnd_plugin_api_context *) api_context;

	rdpsnd_plugin_api_disconnect(api_context);

	CloseHandle(context->hWaitEvent);

	free(api_context);
}

BOOL rdpsnd_plugin_api_connect(void *api_context)
{
	rdpsnd_plugin_api_context *context;

	char szPipeName[64];

	if (!api_context) return FALSE;

	context = (rdpsnd_plugin_api_context *) api_context;

	if (context->rpcClient) return TRUE;

	/* Construct the pipe name. */
	sprintf(szPipeName, "freerds-channel-rdpsnd-%lu", channel_utils_get_x11_display_num());

	/* Connect to the named pipe. */
	context->rpcClient = freerds_rpc_client_new(szPipeName);

	if (context->rpcClient)
	{
		context->rpcClient->custom = context;

		context->rpcClient->ConnectionClosed = rdpsnd_plugin_api_connection_closed;
		context->rpcClient->MessageReceived = rdpsnd_plugin_api_message_received;

		if (freerds_rpc_client_start(context->rpcClient) < 0)
		{
			freerds_rpc_client_free(context->rpcClient);
			context->rpcClient = NULL;
		}
	}

	if (!context->rpcClient)
	{
		WLog_Print(context->log, WLOG_ERROR, "Cannot connect to named pipe '%s'", szPipeName);

		return FALSE;
	}

	return TRUE;
}

BOOL rdpsnd_plugin_api_disconnect(void *api_context)
{
	rdpsnd_plugin_api_context *context;

	if (!api_context) return FALSE;

	context = (rdpsnd_plugin_api_context *) api_context;

	if (!context->rpcClient) return FALSE;

	/* Close the named pipe. */
	if (context->rpcClient)
	{
		freerds_rpc_client_free(context->rpcClient);
		context->rpcClient = NULL;
	}

	return TRUE;
}

BOOL rdpsnd_plugin_api_play_audio(void *api_context, BYTE *buffer, UINT32 length)
{
	rdpsnd_plugin_api_context *context;

	size_t size;
	wStream *request;
	wStream *response;

	if (!api_context) return FALSE;

	context = (rdpsnd_plugin_api_context *) api_context;

	/* Send the audio to the client. */
	size = sizeof(UINT16) + sizeof(UINT32) + length;
	request = Stream_New(NULL, size);
	if (!request)
	{
		WLog_Print(context->log, WLOG_ERROR, "Memory allocation error");

		return FALSE;
	}

	Stream_Write_UINT16(request, RDPSND_PLUGIN_PLAY_AUDIO_REQUEST);
	Stream_Write_UINT32(request, length);
	Stream_Write(request, buffer, length);

	if (!rdpsnd_plugin_api_send_request(context, request)/* ||
		!rdpsnd_plugin_api_wait_response(context, &response)*/)
	{
		WLog_Print(context->log, WLOG_ERROR, "Could not send audio");

		Stream_Free(request, TRUE);

		return FALSE;
	}

	Stream_Free(request, TRUE);
	//Stream_Free(response, TRUE);

	return TRUE;
}
