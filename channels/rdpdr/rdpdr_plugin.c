/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server plugin for RDPDR
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

#include <winpr/wlog.h>
#include <winpr/wtypes.h>

#include <freerdp/server/rdpdr.h>

#include "channel_plugin.h"

typedef struct
{
	wLog *log;

	HANDLE hVC;
	HANDLE hThread;
	DWORD dwThreadId;

	/* Used to implement the RDPDR protocol. */
	RdpdrServerContext *rdpdrServer;
} RDPDR_PLUGIN_CONTEXT;


/***************************************
 * Constructor/Destructor
 **************************************/

static RDPDR_PLUGIN_CONTEXT *rdpdr_plugin_context_new()
{
	RDPDR_PLUGIN_CONTEXT *context;

	context = (RDPDR_PLUGIN_CONTEXT *) malloc(sizeof(RDPDR_PLUGIN_CONTEXT));
	if (!context) return NULL;

	ZeroMemory(context, sizeof(RDPDR_PLUGIN_CONTEXT));

	return context;
}

static void rdpdr_plugin_context_free(RDPDR_PLUGIN_CONTEXT *context)
{
	free(context);
}


/***************************************
 * Plugin Initialization/Termination
 **************************************/

static BOOL rdpdr_plugin_on_plugin_initialize(VCPlugin *plugin)
{
	RDPDR_PLUGIN_CONTEXT *context;

	context = rdpdr_plugin_context_new();
	if (!context) return FALSE;

	context->log = WLog_Get("freerds.channels.rdpdr");

	plugin->context = (void *) context;

	return TRUE;
}

static void rdpdr_plugin_on_plugin_terminate(VCPlugin *plugin)
{
	RDPDR_PLUGIN_CONTEXT *context;

	context = (RDPDR_PLUGIN_CONTEXT *) plugin->context;

	rdpdr_plugin_context_free(context);
}


/***************************************
 * Terminal Service Event Handlers
 **************************************/

static void rdpdr_plugin_on_session_create(VCPlugin *plugin)
{
	RDPDR_PLUGIN_CONTEXT *context;

	context = (RDPDR_PLUGIN_CONTEXT *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_create");
}

static void rdpdr_plugin_on_session_delete(VCPlugin *plugin)
{
	RDPDR_PLUGIN_CONTEXT *context;

	context = (RDPDR_PLUGIN_CONTEXT *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_delete");
}

static void rdpdr_plugin_on_session_connect(VCPlugin *plugin)
{
	RDPDR_PLUGIN_CONTEXT *context;

	context = (RDPDR_PLUGIN_CONTEXT *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_connect");

	if (!context->rdpdrServer)
	{
		RdpdrServerContext *rdpdrServer;

		rdpdrServer = rdpdr_server_context_new(WTS_CURRENT_SERVER_HANDLE);
		if (!rdpdrServer)
		{
			WLog_Print(context->log, WLOG_ERROR, "Failed to create RDPDR server context");
			return;
		}

		context->rdpdrServer = rdpdrServer;
	}
}

static void rdpdr_plugin_on_session_disconnect(VCPlugin *plugin)
{
	RDPDR_PLUGIN_CONTEXT *context;

	context = (RDPDR_PLUGIN_CONTEXT *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_disconnect");

	if (context->rdpdrServer)
	{
		rdpdr_server_context_free(context->rdpdrServer);
		context->rdpdrServer = NULL;
	}
}


/***************************************
 * Plugin Entry Point
 **************************************/

BOOL VCPluginEntry(VCPlugin *plugin)
{
	plugin->name = "RDPDR";

	plugin->OnPluginInitialize = rdpdr_plugin_on_plugin_initialize;
	plugin->OnPluginTerminate = rdpdr_plugin_on_plugin_terminate;

	plugin->OnSessionCreate = rdpdr_plugin_on_session_create;
	plugin->OnSessionDelete = rdpdr_plugin_on_session_delete;
	plugin->OnSessionConnect = rdpdr_plugin_on_session_connect;
	plugin->OnSessionDisconnect = rdpdr_plugin_on_session_disconnect;

	return TRUE;
}
