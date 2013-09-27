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

#include <freerdp/freerdp.h>
#include <freerdp/settings.h>
#include <freerdp/constants.h>

#include <freerdp/gdi/gdi.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/channels/channels.h>

#include "rdp_channels.h"

#include "rdp_client.h"

void rds_begin_paint(rdpContext* context)
{

}

void rds_end_paint(rdpContext* context)
{

}

void rds_surface_frame_marker(rdpContext* context, SURFACE_FRAME_MARKER* surfaceFrameMarker)
{

}

void rds_OnConnectionResultEventHandler(rdpContext* context, ConnectionResultEventArgs* e)
{
	printf("OnConnectionResult: %d\n", e->result);
}

BOOL rds_pre_connect(freerdp* instance)
{
	rdpContext* context = instance->context;

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

	rds = (rdsContext*) instance->context;

	flags = 0;
	flags |= CLRCONV_ALPHA;
	flags |= CLRBUF_32BPP;

	gdi_init(instance, flags, NULL);
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
	wMessage message;
	wMessageQueue* queue;
	freerdp* instance = (freerdp*) arg;

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

	ExitThread(0);
	return NULL;
}

void* rds_client_thread(void* arg)
{
	rdsContext* rds;
	rdpContext* context;
	freerdp* instance = (freerdp*) arg;

	context = (rdpContext*) instance->context;
	rds = (rdsContext*) context;

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
	queue = freerdp_get_message_queue(context->instance, FREERDP_UPDATE_MESSAGE_QUEUE);

	MessageQueue_PostQuit(queue, 0);

	SetEvent(rds->StopEvent);

	WaitForSingleObject(rds->UpdateThread, INFINITE);
	CloseHandle(rds->UpdateThread);

	WaitForSingleObject(rds->ChannelsThread, INFINITE);
	CloseHandle(rds->ChannelsThread);

	return 0;
}

int rds_freerdp_client_new(freerdp* instance, rdpContext* context)
{
	rdsContext* rds;
	rdpSettings* settings;

	rds = (rdsContext*) instance->context;

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

	return 0;
}

void rds_freerdp_client_free(freerdp* instance, rdpContext* context)
{

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
