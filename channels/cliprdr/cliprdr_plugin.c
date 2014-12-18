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

#include <X11/Xlib.h>

#include <freerdp/server/cliprdr.h>

#include "channel_plugin.h"

typedef struct
{
	wLog* log;

	HANDLE Thread;
	HANDLE StopEvent;
	HANDLE X11Event;
	HANDLE ChannelEvent;
	CliprdrServerContext* cliprdr;

	GC gc;
	int xfds;
	int depth;
	Display* display;
	Screen* screen;
	Visual* visual;
	Window root_window;
	int screen_number;
	unsigned long border;
	unsigned long background;

} CLIPRDR_PLUGIN_CONTEXT;

static int freerds_cliprdr_client_capabilities(CliprdrServerContext* context, CLIPRDR_CAPABILITIES* capabilities)
{
	return 1;
}

static int freerds_cliprdr_client_format_list(CliprdrServerContext* context, CLIPRDR_FORMAT_LIST* formatList)
{
	CLIPRDR_FORMAT_LIST_RESPONSE formatListResponse;

	formatListResponse.msgType = CB_FORMAT_LIST_RESPONSE;
	formatListResponse.msgFlags = CB_RESPONSE_OK;
	formatListResponse.dataLen = 0;

	context->ServerFormatListResponse(context, &formatListResponse);

	return 1;
}

static int freerds_cliprdr_client_format_list_response(CliprdrServerContext* context, CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
	return 1;
}

static int freerds_cliprdr_client_format_data_request(CliprdrServerContext* context, CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
	return 1;
}

static int freerds_cliprdr_client_format_data_response(CliprdrServerContext* context, CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
	return 1;
}

static int freerds_cliprdr_server_init(CliprdrServerContext* context)
{
	UINT32 generalFlags;
	CLIPRDR_CAPABILITIES capabilities;
	CLIPRDR_MONITOR_READY monitorReady;
	CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet;

	ZeroMemory(&capabilities, sizeof(capabilities));
	ZeroMemory(&monitorReady, sizeof(monitorReady));

	generalFlags = 0;
	generalFlags |= CB_USE_LONG_FORMAT_NAMES;

	capabilities.msgType = CB_CLIP_CAPS;
	capabilities.msgFlags = 0;
	capabilities.dataLen = 4 + CB_CAPSTYPE_GENERAL_LEN;

	capabilities.cCapabilitiesSets = 1;
	capabilities.capabilitySets = (CLIPRDR_CAPABILITY_SET*) &generalCapabilitySet;

	generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
	generalCapabilitySet.capabilitySetLength = CB_CAPSTYPE_GENERAL_LEN;
	generalCapabilitySet.version = CB_CAPS_VERSION_2;
	generalCapabilitySet.generalFlags = generalFlags;

	context->ServerCapabilities(context, &capabilities);
	context->MonitorReady(context, &monitorReady);

	return 1;
}

static void* freerds_cliprdr_server_thread(CLIPRDR_PLUGIN_CONTEXT* context)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	HANDLE ChannelEvent;
	CliprdrServerContext* cliprdr = context->cliprdr;

	ChannelEvent = cliprdr->GetEventHandle(cliprdr);

	nCount = 0;
	events[nCount++] = context->StopEvent;
	events[nCount++] = ChannelEvent;

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(context->StopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(ChannelEvent, 0) == WAIT_OBJECT_0)
		{
			if (cliprdr->CheckEventHandle(cliprdr) < 0)
				break;
		}
	}

	return NULL;
}

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

	context->display = XOpenDisplay(NULL);

	if (!context->display)
	{
		WLog_Print(context->log, WLOG_ERROR, "Cannot open display");
		return FALSE;
	}

	context->xfds = ConnectionNumber(context->display);
	context->X11Event = CreateFileDescriptorEvent(NULL, FALSE, FALSE, context->xfds);

	context->screen_number = DefaultScreen(context->display);
	context->screen = ScreenOfDisplay(context->display, context->screen_number);
	context->visual = DefaultVisual(context->display, context->screen_number);
	context->gc = DefaultGC(context->display, context->screen_number);
	context->depth = DefaultDepthOfScreen(context->screen);
	context->root_window = RootWindow(context->display, context->screen_number);
	context->border = BlackPixel(context->display, context->screen_number);
	context->background = WhitePixel(context->display, context->screen_number);

	return TRUE;
}

static void cliprdr_plugin_on_plugin_terminate(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	XCloseDisplay(context->display);
	CloseHandle(context->X11Event);

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
	CliprdrServerContext* cliprdr;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_connect");

	if (!context->cliprdr)
	{
		cliprdr = cliprdr_server_context_new(WTS_CURRENT_SERVER_HANDLE);

		if (!cliprdr)
		{
			WLog_Print(context->log, WLOG_ERROR, "Failed to create CLIPRDR server context");
			return;
		}

		context->cliprdr = cliprdr;
		cliprdr->custom = (void*) context;

		cliprdr->ClientCapabilities = freerds_cliprdr_client_capabilities;
		cliprdr->ClientFormatList = freerds_cliprdr_client_format_list;
		cliprdr->ClientFormatListResponse = freerds_cliprdr_client_format_list_response;
		cliprdr->ClientFormatDataRequest = freerds_cliprdr_client_format_data_request;
		cliprdr->ClientFormatDataResponse = freerds_cliprdr_client_format_data_response;

		cliprdr->Open(cliprdr);
		freerds_cliprdr_server_init(cliprdr);

		context->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		context->Thread = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE) freerds_cliprdr_server_thread, (void*) context, 0, NULL);
	}
}

static void cliprdr_plugin_on_session_disconnect(VCPlugin* plugin)
{
	CLIPRDR_PLUGIN_CONTEXT* context;

	context = (CLIPRDR_PLUGIN_CONTEXT*) plugin->context;

	if (!context)
		return;

	WLog_Print(context->log, WLOG_DEBUG, "on_session_disconnect");

	if (context->StopEvent)
	{
		SetEvent(context->StopEvent);
		WaitForSingleObject(context->Thread, INFINITE);
		CloseHandle(context->Thread);
		CloseHandle(context->StopEvent);
	}

	if (context->cliprdr)
	{
		cliprdr_server_context_free(context->cliprdr);
		context->cliprdr = NULL;
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
