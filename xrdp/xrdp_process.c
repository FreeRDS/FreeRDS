/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
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
 *
 * main rdp process
 */

#include "xrdp.h"

#include <winpr/crt.h>
#include <winpr/file.h>
#include <winpr/path.h>
#include <winpr/synch.h>
#include <winpr/thread.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/signal.h>

#include "makecert.h"

#include "xrdp_channels.h"

void xrdp_peer_context_new(freerdp_peer* client, xrdpSession* context)
{
	rdpSettings* settings = client->settings;

	settings->ColorDepth = 32;
	settings->RemoteFxCodec = TRUE;
	settings->BitmapCacheV3Enabled = TRUE;
	settings->FrameMarkerCommandEnabled = TRUE;

	libxrdp_session_init(context, settings);
	context->client = client;

	context->vcm = WTSCreateVirtualChannelManager(client);
}

void xrdp_peer_context_free(freerdp_peer* client, xrdpSession* context)
{
	libxrdp_session_uninit(context);
	WTSDestroyVirtualChannelManager(context->vcm);
}

xrdpSession* xrdp_process_create(freerdp_peer* client)
{
	xrdpSession* xfp;

	client->ContextSize = sizeof(xrdpSession);
	client->ContextNew = (psPeerContextNew) xrdp_peer_context_new;
	client->ContextFree = (psPeerContextFree) xrdp_peer_context_free;
	freerdp_peer_context_new(client);

	xfp = (xrdpSession*) client->context;

	xfp->TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	xfp->Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) xrdp_process_main_thread, client, 0, NULL);

	return xfp;
}

void xrdp_process_delete(xrdpSession* self)
{
	CloseHandle(self->TermEvent);
}

HANDLE xrdp_process_get_term_event(xrdpSession* self)
{
	return self->TermEvent;
}

BOOL xrdp_peer_capabilities(freerdp_peer* client)
{
	return TRUE;
}

BOOL xrdp_peer_post_connect(freerdp_peer* client)
{
	UINT32 ColorDepth;
	UINT32 DesktopWidth;
	UINT32 DesktopHeight;
	rdpSettings* settings;
	xrdpSession* session;

	settings = client->settings;
	session = (xrdpSession*) client->context;

	fprintf(stderr, "Client %s is connected", client->hostname);

	if (client->settings->AutoLogonEnabled)
	{
		fprintf(stderr, " and wants to login automatically as %s\\%s",
			client->settings->Domain ? client->settings->Domain : "",
			client->settings->Username);
	}
	fprintf(stderr, "\n");

	DesktopWidth = settings->DesktopWidth;
	DesktopHeight = settings->DesktopHeight;
	ColorDepth = settings->ColorDepth;

	fprintf(stderr, "Client requested desktop: %dx%dx%d\n",
		settings->DesktopWidth, settings->DesktopHeight, settings->ColorDepth);

	if ((DesktopWidth % 4) != 0)
		DesktopWidth += (DesktopWidth % 4);

	if ((DesktopHeight % 4) != 0)
		DesktopHeight += (DesktopHeight % 4);

	if ((DesktopWidth != settings->DesktopWidth) || (DesktopHeight != settings->DesktopHeight)
			|| (ColorDepth != settings->ColorDepth))
	{
		fprintf(stderr, "Resizing desktop to %dx%dx%d\n", DesktopWidth, DesktopHeight, ColorDepth);

		settings->DesktopWidth = DesktopWidth;
		settings->DesktopHeight = DesktopHeight;
		settings->ColorDepth = ColorDepth;

		client->update->DesktopResize(client->update->context);

		return TRUE;
	}

	xrdp_channels_post_connect(session);

	return TRUE;
}

BOOL xrdp_peer_activate(freerdp_peer* client)
{
	rdpSettings* settings;
	xrdpSession* session = (xrdpSession*) client->context;

	settings = client->settings;
	settings->BitmapCacheVersion = 2;

	if (settings->Password)
		settings->AutoLogonEnabled = 1;

	if (settings->RemoteFxCodec || settings->NSCodec)
		session->codecMode = TRUE;

	if (!session->mod)
		session->mod = xrdp_module_new(session);

	printf("Client Activated\n");

	return TRUE;
}

const char* makecert_argv[4] =
{
	"makecert",
	"-rdp",
	"-live",
	"-silent"
};

int makecert_argc = (sizeof(makecert_argv) / sizeof(char*));

int xrdp_generate_certificate(rdpSettings* settings)
{
	char* config_home;
	char* server_file_path;
	MAKECERT_CONTEXT* context;

	config_home = GetKnownPath(KNOWN_PATH_XDG_CONFIG_HOME);

	if (!PathFileExistsA(config_home))
		CreateDirectoryA(config_home, 0);

	free(config_home);

	if (!PathFileExistsA(settings->ConfigPath))
		CreateDirectoryA(settings->ConfigPath, 0);

	server_file_path = GetCombinedPath(settings->ConfigPath, "server");

	if (!PathFileExistsA(server_file_path))
		CreateDirectoryA(server_file_path, 0);

	settings->CertificateFile = GetCombinedPath(server_file_path, "server.crt");
	settings->PrivateKeyFile = GetCombinedPath(server_file_path, "server.key");

	if ((!PathFileExistsA(settings->CertificateFile)) ||
			(!PathFileExistsA(settings->PrivateKeyFile)))
	{
		context = makecert_context_new();

		makecert_context_process(context, makecert_argc, (char**) makecert_argv);

		makecert_context_set_output_file_name(context, "server");

		if (!PathFileExistsA(settings->CertificateFile))
			makecert_context_output_certificate_file(context, server_file_path);

		if (!PathFileExistsA(settings->PrivateKeyFile))
			makecert_context_output_private_key_file(context, server_file_path);

		makecert_context_free(context);
	}

	free(server_file_path);

	return 0;
}

void xrdp_input_synchronize_event(rdpInput* input, UINT32 flags)
{
	xrdpSession* session = (xrdpSession*) input->context;
	xrdpModule* mod = session->mod;

	if (mod)
	{
		if (mod->client->SynchronizeKeyboardEvent)
		{
			mod->client->SynchronizeKeyboardEvent(mod, flags);
		}
	}
}

void xrdp_input_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	xrdpSession* session = (xrdpSession*) input->context;
	xrdpModule* mod = session->mod;

	if (mod)
	{
		if (mod->client->ScancodeKeyboardEvent)
		{
			mod->client->ScancodeKeyboardEvent(mod, flags, code, session->settings->KeyboardType);
		}
	}
}

void xrdp_input_unicode_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	xrdpSession* session = (xrdpSession*) input->context;
	xrdpModule* mod = session->mod;

	if (mod)
	{
		if (mod->client->UnicodeKeyboardEvent)
		{
			mod->client->UnicodeKeyboardEvent(mod, flags, code);
		}
	}
}

void xrdp_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	xrdpSession* session = (xrdpSession*) input->context;
	xrdpModule* mod = session->mod;

	if (mod)
	{
		if (mod->client->MouseEvent)
		{
			mod->client->MouseEvent(mod, flags, x, y);
		}
	}
}

void xrdp_input_extended_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	xrdpSession* session = (xrdpSession*) input->context;
	xrdpModule* mod = session->mod;

	if (mod)
	{
		if (mod->client->ExtendedMouseEvent)
		{
			mod->client->ExtendedMouseEvent(mod, flags, x, y);
		}
	}
}

void xrdp_input_register_callbacks(rdpInput* input)
{
	input->SynchronizeEvent = xrdp_input_synchronize_event;
	input->KeyboardEvent = xrdp_input_keyboard_event;
	input->UnicodeKeyboardEvent = xrdp_input_unicode_keyboard_event;
	input->MouseEvent = xrdp_input_mouse_event;
	input->ExtendedMouseEvent = xrdp_input_extended_mouse_event;
}

void xrdp_update_frame_acknowledge(rdpContext* context, UINT32 frameId)
{
	SURFACE_FRAME* frame;
	xrdpSession* session = (xrdpSession*) context;

	frame = (SURFACE_FRAME*) ListDictionary_GetItemValue(session->FrameList, (void*) (size_t) frameId);

	if (frame)
	{
		ListDictionary_Remove(session->FrameList, (void*) (size_t) frameId);
		free(frame);
	}
}

void* xrdp_process_main_thread(void* arg)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[32];
	HANDLE ClientEvent;
	HANDLE ChannelEvent;
	HANDLE LocalTermEvent;
	HANDLE GlobalTermEvent;
	xrdpSession* session;
	rdpSettings* settings;
	freerdp_peer* client = (freerdp_peer*) arg;

	fprintf(stderr, "We've got a client %s\n", client->hostname);

	session = (xrdpSession*) client->context;
	settings = client->settings;

	xrdp_generate_certificate(settings);

	settings->RdpSecurity = FALSE;
	settings->TlsSecurity = TRUE;
	settings->NlaSecurity = FALSE;

	client->Capabilities = xrdp_peer_capabilities;
	client->PostConnect = xrdp_peer_post_connect;
	client->Activate = xrdp_peer_activate;

	client->Initialize(client);

	xrdp_input_register_callbacks(client->input);

	client->update->SurfaceFrameAcknowledge = xrdp_update_frame_acknowledge;

	ClientEvent = client->GetEventHandle(client);
	ChannelEvent = WTSVirtualChannelManagerGetEventHandle(session->vcm);

	GlobalTermEvent = g_get_term_event();
	LocalTermEvent = session->TermEvent;

	while (1)
	{
		nCount = 0;
		events[nCount++] = ClientEvent;
		events[nCount++] = ChannelEvent;
		events[nCount++] = GlobalTermEvent;
		events[nCount++] = LocalTermEvent;

		if (client->activated)
		{
			if (session->mod)
				session->mod->client->GetEventHandles(session->mod, events, &nCount);
		}

		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(GlobalTermEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(LocalTermEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(ClientEvent, 0) == WAIT_OBJECT_0)
		{
			if (client->CheckFileDescriptor(client) != TRUE)
			{
				fprintf(stderr, "Failed to check freerdp file descriptor\n");
				break;
			}
		}

		if (WaitForSingleObject(ChannelEvent, 0) == WAIT_OBJECT_0)
		{
			if (WTSVirtualChannelManagerCheckFileDescriptor(session->vcm) != TRUE)
			{
				fprintf(stderr, "WTSVirtualChannelManagerCheckFileDescriptor failure\n");
				break;
			}
		}

		if (client->activated)
		{
			if (session->mod)
			{
				if (session->mod->client->CheckEventHandles(session->mod) < 0)
				{
					fprintf(stderr, "ModuleClient->CheckEventHandles failure\n");
					break;
				}
			}
		}
	}

	fprintf(stderr, "Client %s disconnected.\n", client->hostname);

	client->Disconnect(client);

	freerdp_peer_context_free(client);
	freerdp_peer_free(client);

	return NULL;
}

