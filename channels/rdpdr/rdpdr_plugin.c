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

#include "channel_plugin.h"
#include "rdpdr_plugin.h"
#include "rdpdr_fuse.h"

/***************************************
 * Constructor/Destructor
 **************************************/

static RdpdrPluginContext *rdpdr_plugin_context_new()
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) malloc(sizeof(RdpdrPluginContext));
	if (!context) return NULL;

	ZeroMemory(context, sizeof(RdpdrPluginContext));

	return context;
}

static void rdpdr_plugin_context_free(RdpdrPluginContext *context)
{
	free(context);
}


/***************************************
 * RDPDR FUSE Thread
 **************************************/

DWORD rdpdr_plugin_fuse_thread(LPVOID lpParameter)
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) lpParameter;

	for (;;)
	{
		rdpdr_fuse_check_wait_objs();

		Sleep(10);
	}
}


/***************************************
 * Plugin Initialization/Termination
 **************************************/

static BOOL rdpdr_plugin_on_plugin_initialize(VCPlugin *plugin)
{
	RdpdrPluginContext *context;

	context = rdpdr_plugin_context_new();
	if (!context) return FALSE;

	context->log = WLog_Get("freerds.channels.rdpdr");

	plugin->context = (void *) context;

	return TRUE;
}

static void rdpdr_plugin_on_plugin_terminate(VCPlugin *plugin)
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) plugin->context;

	rdpdr_plugin_context_free(context);
}


/***************************************
 * Terminal Service Event Handlers
 **************************************/

static void rdpdr_plugin_on_session_create(VCPlugin *plugin)
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_create");
}

static void rdpdr_plugin_on_session_delete(VCPlugin *plugin)
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_delete");
}

static void rdpdr_plugin_on_session_connect(VCPlugin *plugin)
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_connect");
}

static void rdpdr_plugin_on_session_disconnect(VCPlugin *plugin)
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_disconnect");
}

static void rdpdr_plugin_on_session_logon(VCPlugin *plugin)
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_logon");

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

		rdpdrServer->data = (void *) context;

		rdpdrServer->supportsDrives = TRUE;
		//rdpdrServer->supportsPorts = TRUE;
		//rdpdrServer->supportsPrinters = TRUE;
		//rdpdrServer->supportsSmartcards = TRUE;

		rdpdr_fuse_init(context);

		context->hThread = CreateThread(NULL, 0, rdpdr_plugin_fuse_thread, context, 0, &context->dwThreadId);

		if (rdpdrServer->Start(rdpdrServer) != 0)
		{
			WLog_Print(context->log, WLOG_ERROR, "failed to initialize RDPDR server");
			rdpdr_server_context_free(rdpdrServer);
			return;
		}
	}
}

static void rdpdr_plugin_on_session_logoff(VCPlugin *plugin)
{
	RdpdrPluginContext *context;

	context = (RdpdrPluginContext *) plugin->context;

	if (!context) return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_logoff");

	if (context->rdpdrServer)
	{
		rdpdr_fuse_deinit();

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
	plugin->OnSessionLogon = rdpdr_plugin_on_session_logon;
	plugin->OnSessionLogoff = rdpdr_plugin_on_session_logoff;

	return TRUE;
}
