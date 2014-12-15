/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FreeRDS channel server plugin for CLIPRDR
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

#include <freerdp/server/cliprdr.h>

#include "channel_plugin.h"

typedef struct
{
	wLog* log;

	HANDLE hVC;
	HANDLE hThread;
	DWORD dwThreadId;

	/* Used to implement the CLIPRDR protocol. */
	CliprdrServerContext* cliprdrServer;
} CLIPRDR_PLUGIN_CONTEXT;


/***************************************
 * Constructor/Destructor
 **************************************/

static CLIPRDR_PLUGIN_CONTEXT* cliprdr_plugin_context_new()
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) calloc(1, sizeof(CLIPRDR_PLUGIN_CONTEXT));

	if (!context)
		return NULL;

	return context;
}

static void cliprdr_plugin_context_free(CLIPRDR_PLUGIN_CONTEXT *context)
{
	free(context);
}


/***************************************
 * Plugin Initialization/Termination
 **************************************/

static BOOL cliprdr_plugin_on_plugin_initialize(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = cliprdr_plugin_context_new();

	if (!context)
		return FALSE;

	context->log = WLog_Get("freerds.channels.cliprdr");

	plugin->context = (void*) context;

	return TRUE;
}

static void cliprdr_plugin_on_plugin_terminate(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	cliprdr_plugin_context_free(context);
}


/***************************************
 * Terminal Service Event Handlers
 **************************************/

static void cliprdr_plugin_on_session_create(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_create");
}

static void cliprdr_plugin_on_session_delete(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_delete");
}

static void cliprdr_plugin_on_session_connect(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_connect");

	if (!context->cliprdrServer)
	{
		CliprdrServerContext* cliprdrServer;

		cliprdrServer = cliprdr_server_context_new(WTS_CURRENT_SERVER_HANDLE);

		if (!cliprdrServer)
		{
			WLog_Print(context->log, WLOG_ERROR, "Failed to create CLIPRDR server context");
			return;
		}

		context->cliprdrServer = cliprdrServer;

		cliprdrServer->Start(cliprdrServer);
	}
}

static void cliprdr_plugin_on_session_disconnect(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_disconnect");

	if (context->cliprdrServer)
	{
		cliprdr_server_context_free(context->cliprdrServer);
		context->cliprdrServer = NULL;
	}
}


/***************************************
 * Plugin Entry Point
 **************************************/

BOOL VCPluginEntry(VCPlugin* plugin)
{
	plugin->name = "CLIPRDR";

	plugin->OnPluginInitialize = cliprdr_plugin_on_plugin_initialize;
	plugin->OnPluginTerminate = cliprdr_plugin_on_plugin_terminate;

	plugin->OnSessionCreate = cliprdr_plugin_on_session_create;
	plugin->OnSessionDelete = cliprdr_plugin_on_session_delete;
	plugin->OnSessionConnect = cliprdr_plugin_on_session_connect;
	plugin->OnSessionDisconnect = cliprdr_plugin_on_session_disconnect;

	return TRUE;
}
