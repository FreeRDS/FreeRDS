 /**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * RDP Module Service
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include <freerdp/freerdp.h>
#include <freerdp/settings.h>
#include <freerdp/constants.h>

#include <freerdp/gdi/gdi.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/channels/channels.h>

#include "rdp_channels.h"

#include "rdp_client.h"

int rds_client_synchronize_keyboard_event(rdsModule* module, DWORD flags)
{
	rdpContext* context;
	rdsContext* rds = ((rdsService*) module)->custom;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsClientSynchronizeKeyboardEvent");

	context = (rdpContext*) rds;
	freerdp_input_send_synchronize_event(context->input, flags);

	return 0;
}

int rds_client_scancode_keyboard_event(rdsModule* module, DWORD flags, DWORD code, DWORD keyboardType)
{
	rdpContext* context;
	rdsContext* rds = ((rdsService*) module)->custom;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsClientScancodeKeyboardEvent");

	context = (rdpContext*) rds;
	freerdp_input_send_keyboard_event(context->input, flags, code);

	return 0;
}

int rds_client_virtual_keyboard_event(rdsModule* module, DWORD flags, DWORD code)
{
	DWORD scancode;
	rdpContext* context;
	rdpSettings* settings;
	rdsContext* rds = (rdsContext*) module;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsClientVirtualKeyboardEvent");

	context = (rdpContext*) rds;
	settings = context->settings;

	scancode = GetVirtualScanCodeFromVirtualKeyCode(code, settings->KeyboardType);
	freerdp_input_send_keyboard_event(context->input, flags, scancode);

	return 0;
}

int rds_client_unicode_keyboard_event(rdsModule* module, DWORD flags, DWORD code)
{
	rdpContext* context;
	rdsContext* rds = ((rdsService*) module)->custom;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsClientUnicodeKeyboardEvent");

	context = (rdpContext*) rds;
	freerdp_input_send_unicode_event(context->input, flags, code);

	return 0;
}

int rds_client_mouse_event(rdsModule* module, DWORD flags, DWORD x, DWORD y)
{
	rdpContext* context;
	rdsContext* rds = ((rdsService*) module)->custom;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsClientMouseEvent");

	context = (rdpContext*) rds;
	freerdp_input_send_mouse_event(context->input, flags, x, y);

	return 0;
}

int rds_client_extended_mouse_event(rdsModule* module, DWORD flags, DWORD x, DWORD y)
{
	rdpContext* context;
	rdsContext* rds = ((rdsService*) module)->custom;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsClientExtendedMouseEvent");

	context = (rdpContext*) rds;
	freerdp_input_send_mouse_event(context->input, flags, x, y);

	return 0;
}

int rds_service_accept(rdsService* service)
{
	rdsContext* rds = (rdsContext*) service->custom;
	WLog_Print(rds->log, WLOG_DEBUG, "RdsServiceAccept");
	return 0;
}

int rds_check_shared_framebuffer(rdsContext* rds)
{
	rdsModule* module = (rdsModule*) rds->service;

	if (!rds->framebuffer.fbAttached)
	{
		RDS_MSG_SHARED_FRAMEBUFFER msg;

		msg.attach = 1;
		msg.width = rds->framebuffer.fbWidth;
		msg.height = rds->framebuffer.fbHeight;
		msg.scanline = rds->framebuffer.fbScanline;
		msg.segmentId = rds->framebuffer.fbSegmentId;
		msg.bitsPerPixel = rds->framebuffer.fbBitsPerPixel;
		msg.bytesPerPixel = rds->framebuffer.fbBytesPerPixel;

		msg.type = RDS_SERVER_SHARED_FRAMEBUFFER;
		module->server->SharedFramebuffer(module, &msg);

		rds->framebuffer.fbAttached = 1;
	}

	return 0;
}

void rds_begin_paint(rdpContext* context)
{
	rdsContext* rds;
	rdpGdi* gdi = context->gdi;

	rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsBeginPaint");

	gdi->primary->hdc->hwnd->invalid->null = 1;
	gdi->primary->hdc->hwnd->ninvalid = 0;
}

void rds_end_paint(rdpContext* context)
{
	INT32 x, y;
	UINT32 w, h;
	rdsContext* rds;
	HGDI_RGN invalid;
	HGDI_RGN cinvalid;
	rdsModule* module;
	rdpSettings* settings;
	RDS_MSG_PAINT_RECT msg;
	rdpGdi* gdi = context->gdi;

	rds = (rdsContext*) context;
	module = (rdsModule*) rds->service;
	settings = context->settings;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsEndPaint");

	invalid = gdi->primary->hdc->hwnd->invalid;
	cinvalid = gdi->primary->hdc->hwnd->cinvalid;

	rds_check_shared_framebuffer(rds);

	x = invalid->x;
	y = invalid->y;
	w = invalid->w;
	h = invalid->h;

	if (x < 0)
		x = 0;

	if (x > settings->DesktopWidth - 1)
		x = settings->DesktopWidth - 1;

	w += x % 16;
	x -= x % 16;

	w += w % 16;

	if (x + w > settings->DesktopWidth)
		w = settings->DesktopWidth - x;

	if (y < 0)
		y = 0;

	if (y > settings->DesktopHeight - 1)
		y = settings->DesktopHeight - 1;

	h += y % 16;
	y -= y % 16;

	h += h % 16;

	if (h > settings->DesktopHeight)
		h = settings->DesktopHeight;

	if (y + h > settings->DesktopHeight)
		h = settings->DesktopHeight - y;

	if (w * h < 1)
		return;

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = w;
	msg.nHeight = h;
	msg.nXSrc = 0;
	msg.nYSrc = 0;

	msg.fbSegmentId = rds->framebuffer.fbSegmentId;
	msg.bitmapData = NULL;
	msg.bitmapDataLength = 0;

	msg.type = RDS_SERVER_PAINT_RECT;
	module->server->PaintRect(module, &msg);
}

void rds_surface_frame_marker(rdpContext* context, SURFACE_FRAME_MARKER* surfaceFrameMarker)
{
	rdsContext* rds;

	rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsSurfaceFrameMarker");
}

void rds_OnConnectionResultEventHandler(rdpContext* context, ConnectionResultEventArgs* e)
{
	rdsContext* rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "OnConnectionResult: %d", e->result);
}

BOOL rds_pre_connect(freerdp* instance)
{
	rdsContext* rds;
	rdpContext* context = instance->context;

	rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "RdpPreConnect");

	PubSub_SubscribeConnectionResult(context->pubSub,
			(pConnectionResultEventHandler) rds_OnConnectionResultEventHandler);

	PubSub_SubscribeChannelConnected(context->pubSub,
			(pChannelConnectedEventHandler) rds_OnChannelConnectedEventHandler);

	PubSub_SubscribeChannelDisconnected(context->pubSub,
			(pChannelDisconnectedEventHandler) rds_OnChannelDisconnectedEventHandler);

	freerdp_client_load_addins(context->channels, instance->settings);

	freerdp_channels_pre_connect(context->channels, instance);

	return TRUE;
}

BOOL rds_post_connect(freerdp* instance)
{
	rdpGdi* gdi;
	UINT32 flags;
	rdsContext* rds;
	rdpSettings* settings;

	rds = (rdsContext*) instance->context;
	settings = instance->settings;

	WLog_Print(rds->log, WLOG_DEBUG, "RdpPostConnect");

	flags = 0;
	flags |= CLRCONV_ALPHA;
	flags |= CLRBUF_32BPP;

	rds->framebuffer.fbBitsPerPixel = 32;
	rds->framebuffer.fbBytesPerPixel = 4;

	rds->framebuffer.fbWidth = settings->DesktopWidth;
	rds->framebuffer.fbHeight = settings->DesktopHeight;

	rds->framebuffer.fbScanline = rds->framebuffer.fbWidth * rds->framebuffer.fbBytesPerPixel;
	rds->framebufferSize = rds->framebuffer.fbScanline * rds->framebuffer.fbHeight;

	rds->framebuffer.fbSegmentId = shmget(IPC_PRIVATE, rds->framebufferSize,
			IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	rds->framebuffer.fbSharedMemory = (BYTE*) shmat(rds->framebuffer.fbSegmentId, 0, 0);

	gdi_init(instance, flags, rds->framebuffer.fbSharedMemory);
	gdi = instance->context->gdi;

	instance->update->BeginPaint = rds_begin_paint;
	instance->update->EndPaint = rds_end_paint;
	instance->update->SurfaceFrameMarker = rds_surface_frame_marker;

	freerdp_channels_post_connect(instance->context->channels, instance);

	return TRUE;
}

void* rds_update_thread(void* arg)
{
	int status;
	rdsContext* rds;
	wMessage message;
	rdpContext* context;
	wMessageQueue* queue;
	freerdp* instance = (freerdp*) arg;

	context = (rdpContext*) instance->context;
	rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "Starting update thread");

	status = 1;
	queue = freerdp_get_message_queue(instance, FREERDP_UPDATE_MESSAGE_QUEUE);

	while (MessageQueue_Wait(queue))
	{
		while (MessageQueue_Peek(queue, &message, TRUE))
		{
			status = freerdp_message_queue_process_message(instance, FREERDP_UPDATE_MESSAGE_QUEUE, &message);

			if (!status)
				break;
		}

		if (!status)
			break;
	}

	WLog_Print(rds->log, WLOG_DEBUG, "Terminating update thread");

	ExitThread(0);
	return NULL;
}

void* rds_client_thread(void* arg)
{
	rdsContext* rds;
	rdsModule* module;
	rdpContext* context;
	const char* endpoint = "RDP";
	freerdp* instance = (freerdp*) arg;

	context = (rdpContext*) instance->context;
	rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "Starting client thread");

	WLog_Print(rds->log, WLOG_DEBUG, "Creating service: SessionId: %d Endpoint: %s",
			(int) rds->SessionId, endpoint);

	rds->service = freerds_service_new(rds->SessionId, endpoint);

	module = (rdsModule*) rds->service;

	rds->service->custom = (void*) rds;
	rds->service->Accept = rds_service_accept;

	module->client->SynchronizeKeyboardEvent = rds_client_synchronize_keyboard_event;
	module->client->ScancodeKeyboardEvent = rds_client_scancode_keyboard_event;
	module->client->VirtualKeyboardEvent = rds_client_virtual_keyboard_event;
	module->client->UnicodeKeyboardEvent = rds_client_unicode_keyboard_event;
	module->client->MouseEvent = rds_client_mouse_event;
	module->client->ExtendedMouseEvent = rds_client_extended_mouse_event;

	WLog_Print(rds->log, WLOG_DEBUG, "Starting %s service", endpoint);

	freerds_service_start(rds->service);

	WLog_Print(rds->log, WLOG_DEBUG, "Initiating connection to RDP server");

	freerdp_connect(instance);

	rds->UpdateThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) rds_update_thread,
			instance, 0, NULL);

	rds->ChannelsThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) rds_channels_thread,
			instance, 0, NULL);

	WaitForSingleObject(rds->UpdateThread, INFINITE);

	CloseHandle(rds->UpdateThread);
	freerdp_free(instance);

	WLog_Print(rds->log, WLOG_DEBUG, "Terminating client thread");

	ExitThread(0);
	return NULL;
}

/**
 * Client Interface
 */

void rds_freerdp_client_global_init()
{
	freerdp_channels_global_init();
}

void rds_freerdp_client_global_uninit()
{
	freerdp_channels_global_uninit();
}

int rds_freerdp_client_start(rdpContext* context)
{
	rdsContext* rds;
	freerdp* instance = context->instance;

	rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsRdpClientStart");

	rds->thread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) rds_client_thread,
			instance, 0, NULL);

	return 0;
}

int rds_freerdp_client_stop(rdpContext* context)
{
	rdsContext* rds;
	wMessageQueue* queue;

	rds = (rdsContext*) context;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsRdpClientStop");

	queue = freerdp_get_message_queue(context->instance, FREERDP_UPDATE_MESSAGE_QUEUE);

	MessageQueue_PostQuit(queue, 0);

	SetEvent(rds->StopEvent);

	WaitForSingleObject(rds->UpdateThread, INFINITE);
	CloseHandle(rds->UpdateThread);

	WaitForSingleObject(rds->ChannelsThread, INFINITE);
	CloseHandle(rds->ChannelsThread);

	freerds_service_stop(rds->service);

	return 0;
}

int rds_freerdp_client_new(freerdp* instance, rdpContext* context)
{
	rdsContext* rds;
	rdpSettings* settings;

	WLog_Init();

	rds = (rdsContext*) instance->context;

	rds->log = WLog_Get("com.freerds.module.rdp.service");
	WLog_OpenAppender(rds->log);

	WLog_SetLogLevel(rds->log, WLOG_DEBUG);

	WLog_Print(rds->log, WLOG_DEBUG, "RdsRdpClientNew");

	instance->PreConnect = rds_pre_connect;
	instance->PostConnect = rds_post_connect;
	instance->ReceiveChannelData = rds_receive_channel_data;

	context->channels = freerdp_channels_new();

	rds->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	settings = instance->settings;
	rds->settings = instance->context->settings;

	settings->OsMajorType = OSMAJORTYPE_UNIX;
	settings->OsMinorType = OSMINORTYPE_NATIVE_XSERVER;

	settings->SoftwareGdi = TRUE;
	settings->OrderSupport[NEG_DSTBLT_INDEX] = TRUE;
	settings->OrderSupport[NEG_PATBLT_INDEX] = TRUE;
	settings->OrderSupport[NEG_SCRBLT_INDEX] = TRUE;
	settings->OrderSupport[NEG_OPAQUE_RECT_INDEX] = TRUE;
	settings->OrderSupport[NEG_DRAWNINEGRID_INDEX] = FALSE;
	settings->OrderSupport[NEG_MULTIDSTBLT_INDEX] = FALSE;
	settings->OrderSupport[NEG_MULTIPATBLT_INDEX] = FALSE;
	settings->OrderSupport[NEG_MULTISCRBLT_INDEX] = FALSE;
	settings->OrderSupport[NEG_MULTIOPAQUERECT_INDEX] = TRUE;
	settings->OrderSupport[NEG_MULTI_DRAWNINEGRID_INDEX] = FALSE;
	settings->OrderSupport[NEG_LINETO_INDEX] = TRUE;
	settings->OrderSupport[NEG_POLYLINE_INDEX] = TRUE;
	settings->OrderSupport[NEG_MEMBLT_INDEX] = settings->BitmapCacheEnabled;
	settings->OrderSupport[NEG_MEM3BLT_INDEX] = (settings->SoftwareGdi) ? TRUE : FALSE;
	settings->OrderSupport[NEG_MEMBLT_V2_INDEX] = settings->BitmapCacheEnabled;
	settings->OrderSupport[NEG_MEM3BLT_V2_INDEX] = FALSE;
	settings->OrderSupport[NEG_SAVEBITMAP_INDEX] = FALSE;
	settings->OrderSupport[NEG_GLYPH_INDEX_INDEX] = TRUE;
	settings->OrderSupport[NEG_FAST_INDEX_INDEX] = TRUE;
	settings->OrderSupport[NEG_FAST_GLYPH_INDEX] = TRUE;
	settings->OrderSupport[NEG_POLYGON_SC_INDEX] = (settings->SoftwareGdi) ? FALSE : TRUE;
	settings->OrderSupport[NEG_POLYGON_CB_INDEX] = (settings->SoftwareGdi) ? FALSE : TRUE;
	settings->OrderSupport[NEG_ELLIPSE_SC_INDEX] = FALSE;
	settings->OrderSupport[NEG_ELLIPSE_CB_INDEX] = FALSE;

	settings->AsyncTransport = TRUE;
	settings->AsyncChannels = TRUE;
	settings->AsyncUpdate = TRUE;

	rds->SessionId = 10;

	return 0;
}

void rds_freerdp_client_free(freerdp* instance, rdpContext* context)
{
	rdsContext* rds;

	rds = (rdsContext*) instance->context;

	WLog_Print(rds->log, WLOG_DEBUG, "RdsRdpClientFree");

	if (rds->service)
	{
		freerds_service_free(rds->service);
		rds->service = NULL;
	}

	WLog_Uninit();
}

int RDS_RdpClientEntry(RDP_CLIENT_ENTRY_POINTS* pEntryPoints)
{
	pEntryPoints->Version = 1;
	pEntryPoints->Size = sizeof(RDP_CLIENT_ENTRY_POINTS_V1);

	pEntryPoints->GlobalInit = rds_freerdp_client_global_init;
	pEntryPoints->GlobalUninit = rds_freerdp_client_global_uninit;

	pEntryPoints->ContextSize = sizeof(rdsContext);
	pEntryPoints->ClientNew = rds_freerdp_client_new;
	pEntryPoints->ClientFree = rds_freerdp_client_free;

	pEntryPoints->ClientStart = rds_freerdp_client_start;
	pEntryPoints->ClientStop = rds_freerdp_client_stop;

	return 0;
}
