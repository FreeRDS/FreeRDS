/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server plugin for RDPSND
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

#include <winpr/stream.h>
#include <winpr/sysinfo.h>
#include <winpr/wlog.h>
#include <winpr/wtypes.h>

#include <freerdp/server/rdpsnd.h>

#include <freerds/channel_plugin.h>
#include <freerds/rpc.h>

#include "rdpsnd_plugin_msgs.h"

typedef struct
{
	wLog *log;

	/* Used to accept connections over a named pipe. */
	rdsRpcServer *rpcServer;

	/* Used to implement the RDPSND protocol. */
	RdpsndServerContext *rdpsndServer;
}
RdpsndPluginContext;

static AUDIO_FORMAT g_server_audio_formats[] =
{
	/* PCM, 2 channels, 44.1 Khz, 16 bits/sample */
	{
		WAVE_FORMAT_PCM,
		2,
		44100,
		44100 * 4,
		4,
		16,
		0,
		NULL
	},
#if 0
	/* PCM, 2 channels, 22050 Hz, 16 bits/sample */
	{
		WAVE_FORMAT_PCM,
		2,
		22050,
		22050 * 4,
		4,
		16,
		0,
		NULL
	}
#endif
};

static int g_num_server_audio_formats = sizeof(g_server_audio_formats) / sizeof(AUDIO_FORMAT);


/***************************************
 * Constructor/Destructor
 **************************************/

static RdpsndPluginContext *rdpsnd_plugin_context_new()
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) malloc(sizeof(RdpsndPluginContext));
	if (!context) return NULL;

	ZeroMemory(context, sizeof(RdpsndPluginContext));

	return context;
}

static void rdpsnd_plugin_context_free(RdpsndPluginContext *context)
{
	free(context);
}


/***************************************
 * RPC Handlers
 **************************************/

static BOOL rdpsnd_plugin_handle_play_audio(rdsRpcClient *rpcClient, wStream *request)
{
	RdpsndPluginContext *context;
	//wStream *response;
	UINT32 audioSize;
	BYTE *audioData;
	int nFrames;
	UINT16 wTimestamp;
	BOOL status;

	context = (RdpsndPluginContext *) rpcClient->custom;

	/* Parse the request. */
	Stream_Read_UINT32(request, audioSize);
	audioData = Stream_Pointer(request);

	/* Play the audio. */
	nFrames = audioSize / (2 * sizeof(UINT16));
	wTimestamp = (UINT16) GetTickCount();

	WLog_Print(context->log, WLOG_DEBUG,
		"play audio - audioData=%p, audioSize=%lu, nFrames=%d, wTimestamp=%u",
		audioData, audioSize, nFrames, wTimestamp);

	//winpr_HexDump("audioData", WLOG_DEBUG, audioData, audioSize);

	status = context->rdpsndServer->SendSamples(context->rdpsndServer, audioData, nFrames, wTimestamp);

	WLog_Print(context->log, WLOG_DEBUG, "play audio - status=%d", status ? 1 : 0);

#if 0
	/* Construct the response. */
	response = Stream_New(NULL, sizeof(UINT16));

	if (!response)
	{
		WLog_Print(context->log, WLOG_ERROR, "memory allocation error");
		return FALSE;
	}

	Stream_Write_UINT16(response, RDPSND_PLUGIN_PLAY_AUDIO_RESPONSE);

	/* Send the response. */
	if (freerds_rpc_client_send_message(rpcClient, Stream_Buffer(response), Stream_GetPosition(response)) < 0)
	{
		WLog_Print(context->log, WLOG_ERROR, "error sending response");
		Stream_Free(response, TRUE);
		return FALSE;
	}

	Stream_Free(response, TRUE);
#endif

	return TRUE;	
}


/***************************************
 * RPC Callbacks
 **************************************/

static int rdpsnd_plugin_connection_accepted(rdsRpcClient *rpcClient)
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) rpcClient->custom;

	WLog_Print(context->log, WLOG_DEBUG, "connection_accepted");

	return 0;
}

static int rdpsnd_plugin_connection_closed(rdsRpcClient *rpcClient)
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) rpcClient->custom;

	WLog_Print(context->log, WLOG_DEBUG, "connection_closed");

	return 0;
}

static int rdpsnd_plugin_message_received(rdsRpcClient *rpcClient, BYTE *buffer, UINT32 length)
{
	RdpsndPluginContext *context;
	UINT16 messageId;
	BOOL success;
	wStream *s;

	success = FALSE;

	context = (RdpsndPluginContext *) rpcClient->custom;

	WLog_Print(context->log, WLOG_DEBUG, "message_received");

	s = Stream_New(buffer, length);

	Stream_Read_UINT16(s, messageId);

	switch (messageId)
	{
		case RDPSND_PLUGIN_PLAY_AUDIO_REQUEST:
			success = rdpsnd_plugin_handle_play_audio(rpcClient, s);
			break;

		default:
			WLog_Print(context->log, WLOG_ERROR, "unrecognized message %d", messageId);
			break;
	}

	Stream_Free(s, FALSE);

	return success ? 0 : -1;
}


/***************************************
 * RDPSND Server Callbacks
 **************************************/

static void rdpsnd_plugin_server_activated(RdpsndServerContext *rdpsndServer)
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) rdpsndServer->data;

	WLog_Print(context->log, WLOG_DEBUG, "RDPSND server activated");

	WLog_Print(context->log, WLOG_INFO, "num_client_formats=%d, num_server_formats=%d", rdpsndServer->num_client_formats, rdpsndServer->num_server_formats);

	/* Select the first available client format. */
	if (rdpsndServer->num_client_formats <= 0)
	{
		WLog_Print(context->log, WLOG_ERROR, "no available RDPSND client formats");
		rdpsnd_server_context_free(rdpsndServer);
		return;
	}

	if (!rdpsndServer->SelectFormat(rdpsndServer, 0))
	{
		WLog_Print(context->log, WLOG_ERROR, "failed to select RDPSND client format");
		rdpsnd_server_context_free(rdpsndServer);
		return;
	}

	WLog_Print(context->log, WLOG_DEBUG,
		"RDPSND server selected format - format=%u, nchannels=%d, samplerate=%d, bitspersample=%u",
		rdpsndServer->client_formats[0].wFormatTag,
		rdpsndServer->client_formats[0].nChannels,
		rdpsndServer->client_formats[0].nSamplesPerSec,
		rdpsndServer->client_formats[0].wBitsPerSample);

	/* Set the volume to maximum volume. */
	if (!rdpsndServer->SetVolume(context->rdpsndServer, 0xFFFF, 0xFFFF))
	{
		WLog_Print(context->log, WLOG_ERROR, "failed to set RDPSND volume");
		rdpsnd_server_context_free(rdpsndServer);
		return;
	}
}


/***************************************
 * Plugin Initialization/Termination
 **************************************/

static BOOL rdpsnd_plugin_on_plugin_initialize(VCPlugin *plugin)
{
	RdpsndPluginContext *context;

	context = rdpsnd_plugin_context_new();
	if (!context) return FALSE;

	context->log = WLog_Get("freerds.channels.rdpsnd");

	plugin->context = (void *) context;

	return TRUE;
}

static void rdpsnd_plugin_on_plugin_terminate(VCPlugin *plugin)
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) plugin->context;

	//WLog_Free(context->log);

	rdpsnd_plugin_context_free(context);
}


/***************************************
 * Terminal Service Event Handlers
 **************************************/

static void rdpsnd_plugin_on_session_create(VCPlugin *plugin)
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_create");

	/* Load the pulseaudio sink for FreeRDS. */
	system("pactl load-module module-freerds-sink");
}

static void rdpsnd_plugin_on_session_delete(VCPlugin *plugin)
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_delete");
}

static void rdpsnd_plugin_on_session_connect(VCPlugin *plugin)
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_connect");

	if (!context->rpcServer)
	{
		char szPipeName[128];
		rdsRpcServer *rpcServer;

		sprintf(szPipeName, "freerds-channel-rdpsnd-%lu", plugin->x11DisplayNum);

		WLog_Print(context->log, WLOG_DEBUG, "creating named pipe server on '%s'", szPipeName);

		rpcServer = freerds_rpc_server_new(szPipeName);
		if (!rpcServer)
		{
			WLog_Print(context->log, WLOG_ERROR, "failed to create named pipe '%s'", szPipeName);
			return;
		}

		rpcServer->custom = (void *) context;

		rpcServer->ConnectionAccepted = rdpsnd_plugin_connection_accepted;
		rpcServer->ConnectionClosed = rdpsnd_plugin_connection_closed;
		rpcServer->MessageReceived = rdpsnd_plugin_message_received;

		if (freerds_rpc_server_start(rpcServer) < 0)
		{
			WLog_Print(context->log, WLOG_ERROR, "failed to start named pipe listener '%s", szPipeName);
			freerds_rpc_server_free(rpcServer);
			return;
		}

		context->rpcServer = rpcServer;
	}
			
	if (!context->rdpsndServer)
	{
		RdpsndServerContext *rdpsndServer;

		rdpsndServer = rdpsnd_server_context_new(WTS_CURRENT_SERVER_HANDLE);
		if (!rdpsndServer)
		{
			WLog_Print(context->log, WLOG_ERROR, "failed to create RDPSND server context");
			return;
		}

		rdpsndServer->data = (void *) context;

		rdpsndServer->Activated = rdpsnd_plugin_server_activated;

		rdpsndServer->num_server_formats = g_num_server_audio_formats;
		rdpsndServer->server_formats = g_server_audio_formats;

		rdpsndServer->src_format.wFormatTag = WAVE_FORMAT_PCM;
		rdpsndServer->src_format.nChannels = 2;
		rdpsndServer->src_format.nSamplesPerSec = 44100;
		rdpsndServer->src_format.nAvgBytesPerSec = 176400;
		rdpsndServer->src_format.nBlockAlign = 4;
		rdpsndServer->src_format.wBitsPerSample = 16;
		rdpsndServer->src_format.cbSize = 0;
		rdpsndServer->src_format.data = NULL;

		if (!rdpsndServer->Initialize(rdpsndServer, TRUE))
		{
			WLog_Print(context->log, WLOG_ERROR, "failed to initialize RDPSND server");
			rdpsnd_server_context_free(rdpsndServer);
			return;
		}

		context->rdpsndServer = rdpsndServer;
	}
}

static void rdpsnd_plugin_on_session_disconnect(VCPlugin *plugin)
{
	RdpsndPluginContext *context;

	context = (RdpsndPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_disconnect");

	if (context->rpcServer)
	{
		freerds_rpc_server_free(context->rpcServer);
		context->rpcServer = NULL;
	}

	if (context->rdpsndServer)
	{
		rdpsnd_server_context_free(context->rdpsndServer);
		context->rdpsndServer = NULL;
	}
}


/***************************************
 * Plugin Entry Point
 **************************************/

BOOL VCPluginEntry(VCPlugin *plugin)
{
	plugin->name = "RDPSND";

	plugin->OnPluginInitialize = rdpsnd_plugin_on_plugin_initialize;
	plugin->OnPluginTerminate = rdpsnd_plugin_on_plugin_terminate;

	plugin->OnSessionCreate = rdpsnd_plugin_on_session_create;
	plugin->OnSessionDelete = rdpsnd_plugin_on_session_delete;
	plugin->OnSessionConnect = rdpsnd_plugin_on_session_connect;
	plugin->OnSessionDisconnect = rdpsnd_plugin_on_session_disconnect;

	return TRUE;
}
