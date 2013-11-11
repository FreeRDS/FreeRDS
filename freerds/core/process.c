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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "freerds.h"

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

#include <freerds/auth.h>
#include <freerds/module_connector.h>
#include <freerds/icp_client_stubs.h>

#include "makecert.h"

#include "channels.h"

void freerds_peer_context_new(freerdp_peer* client, rdsConnection* context)
{
	rdpSettings* settings = client->settings;

	settings->ColorDepth = 32;
	settings->RemoteFxCodec = TRUE;
	settings->BitmapCacheV3Enabled = TRUE;
	settings->FrameMarkerCommandEnabled = TRUE;

	freerds_connection_init(context, settings);
	context->client = client;

	context->vcm = WTSCreateVirtualChannelManager(client);
}

void freerds_peer_context_free(freerdp_peer* client, rdsConnection* context)
{
	freerds_connection_uninit(context);
	WTSDestroyVirtualChannelManager(context->vcm);
}

rdsConnection* freerds_connection_create(freerdp_peer* client)
{
	rdsConnection* xfp;

	client->ContextSize = sizeof(rdsConnection);
	client->ContextNew = (psPeerContextNew) freerds_peer_context_new;
	client->ContextFree = (psPeerContextFree) freerds_peer_context_free;
	freerdp_peer_context_new(client);

	xfp = (rdsConnection*) client->context;

	xfp->TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	xfp->Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) freerds_connection_main_thread, client, 0, NULL);

	return xfp;
}

void freerds_connection_delete(rdsConnection* self)
{
	CloseHandle(self->TermEvent);
}

HANDLE freerds_connection_get_term_event(rdsConnection* self)
{
	return self->TermEvent;
}

BOOL freerds_peer_capabilities(freerdp_peer* client)
{
	return TRUE;
}

BOOL freerds_peer_post_connect(freerdp_peer* client)
{
	UINT32 ColorDepth;
	UINT32 DesktopWidth;
	UINT32 DesktopHeight;
	rdpSettings* settings;
	rdsConnection* connection;

	settings = client->settings;
	connection = (rdsConnection*) client->context;

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

	freerds_channels_post_connect(connection);

	return TRUE;
}

BOOL freerds_peer_activate(freerdp_peer* client)
{
	int error_code;
	HANDLE hClientPipe;
	rdpSettings* settings;
	rdsConnection* connection = (rdsConnection*) client->context;

	settings = client->settings;
	settings->BitmapCacheVersion = 2;

	if (settings->Password)
		settings->AutoLogonEnabled = 1;

	if (settings->RemoteFxCodec || settings->NSCodec)
		connection->codecMode = TRUE;

	if (!connection->connector)
		connection->connector = freerds_module_connector_new(connection);

	error_code = freerds_icp_GetUserSession(settings->Username, settings->Domain,
			(UINT32*)(&(connection->connector->SessionId)), (&(connection->connector->Endpoint)));

	if (error_code != 0)
	{
		printf("freerds_icp_GetUserSession failed %d\n", error_code);
		return FALSE;
	}

	hClientPipe = freerds_named_pipe_connect(connection->connector->Endpoint, 20);

	if (!hClientPipe)
	{
		fprintf(stderr, "Failed to create named pipe %s\n", connection->connector->Endpoint);
		return FALSE;
	}
	printf("Connected to session %d\n", (int) connection->connector->SessionId);

	connection->connector->hClientPipe = hClientPipe;
	connection->connector->GetEventHandles = freerds_client_get_event_handles;
	connection->connector->CheckEventHandles = freerds_client_check_event_handles;

	connection->connector->ServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) freerds_client_thread,
			(void*) connection->connector, CREATE_SUSPENDED, NULL);

	freerds_client_inbound_connector_init(connection->connector);

	ResumeThread(connection->connector->ServerThread);

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

int freerds_generate_certificate(rdpSettings* settings)
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

void freerds_input_synchronize_event(rdpInput* input, UINT32 flags)
{
	rdsConnection* connection = (rdsConnection*) input->context;
	rdsModuleConnector* connector = connection->connector;

	if (connector)
	{
		if (connector->client->SynchronizeKeyboardEvent)
		{
			connector->client->SynchronizeKeyboardEvent(connector, flags);
		}
	}
}

void freerds_input_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	rdsConnection* connection = (rdsConnection*) input->context;
	rdsModuleConnector* connector = connection->connector;

	if (connector)
	{
		if (connector->client->ScancodeKeyboardEvent)
		{
			connector->client->ScancodeKeyboardEvent(connector, flags, code, connection->settings->KeyboardType);
		}
	}
}

void freerds_input_unicode_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	rdsConnection* connection = (rdsConnection*) input->context;
	rdsModuleConnector* connector = connection->connector;

	if (connector)
	{
		if (connector->client->UnicodeKeyboardEvent)
		{
			connector->client->UnicodeKeyboardEvent(connector, flags, code);
		}
	}
}

void freerds_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	rdsConnection* connection = (rdsConnection*) input->context;
	rdsModuleConnector* connector = connection->connector;

	if (connector)
	{
		if (connector->client->MouseEvent)
		{
			connector->client->MouseEvent(connector, flags, x, y);
		}
	}
}

void freerds_input_extended_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	rdsConnection* connection = (rdsConnection*) input->context;
	rdsModuleConnector* connector = connection->connector;

	if (connector)
	{
		if (connector->client->ExtendedMouseEvent)
		{
			connector->client->ExtendedMouseEvent(connector, flags, x, y);
		}
	}
}

void freerds_input_register_callbacks(rdpInput* input)
{
	input->SynchronizeEvent = freerds_input_synchronize_event;
	input->KeyboardEvent = freerds_input_keyboard_event;
	input->UnicodeKeyboardEvent = freerds_input_unicode_keyboard_event;
	input->MouseEvent = freerds_input_mouse_event;
	input->ExtendedMouseEvent = freerds_input_extended_mouse_event;
}

void freerds_update_frame_acknowledge(rdpContext* context, UINT32 frameId)
{
	SURFACE_FRAME* frame;
	rdsConnection* connection = (rdsConnection*) context;

	frame = (SURFACE_FRAME*) ListDictionary_GetItemValue(connection->FrameList, (void*) (size_t) frameId);

	if (frame)
	{
		ListDictionary_Remove(connection->FrameList, (void*) (size_t) frameId);
		free(frame);
	}
}

void* freerds_connection_main_thread(void* arg)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[32];
	HANDLE ClientEvent;
	HANDLE ChannelEvent;
	HANDLE LocalTermEvent;
	HANDLE GlobalTermEvent;
	rdsConnection* connection;
	rdpSettings* settings;
	rdsModuleConnector* connector;
	freerdp_peer* client = (freerdp_peer*) arg;
	BOOL disconnected = FALSE;

	fprintf(stderr, "We've got a client %s\n", client->hostname);

	connection = (rdsConnection*) client->context;
	settings = client->settings;

	freerds_generate_certificate(settings);

	settings->RdpSecurity = FALSE;
	settings->TlsSecurity = TRUE;
	settings->NlaSecurity = FALSE;

	client->Capabilities = freerds_peer_capabilities;
	client->PostConnect = freerds_peer_post_connect;
	client->Activate = freerds_peer_activate;

	client->Initialize(client);

	freerds_input_register_callbacks(client->input);

	client->update->SurfaceFrameAcknowledge = freerds_update_frame_acknowledge;

	ClientEvent = client->GetEventHandle(client);
	ChannelEvent = WTSVirtualChannelManagerGetEventHandle(connection->vcm);

	GlobalTermEvent = g_get_term_event();
	LocalTermEvent = connection->TermEvent;

	while (1)
	{
		nCount = 0;
		events[nCount++] = ClientEvent;
		events[nCount++] = ChannelEvent;
		events[nCount++] = GlobalTermEvent;
		events[nCount++] = LocalTermEvent;

		if (client->activated)
		{
			connector = (rdsModuleConnector*) connection->connector;

			if (connector)
				connector->GetEventHandles(connection->connector, events, &nCount);
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
			if (WTSVirtualChannelManagerCheckFileDescriptor(connection->vcm) != TRUE)
			{
				fprintf(stderr, "WTSVirtualChannelManagerCheckFileDescriptor failure\n");
				break;
			}
		}

		if (client->activated)
		{
			connector = (rdsModuleConnector*) connection->connector;

			if (connector)
			{
				if (connector->CheckEventHandles(connection->connector) < 0)
				{
					fprintf(stderr, "ModuleClient->CheckEventHandles failure\n");
					break;
				}
			}
		}
	}

	fprintf(stderr, "Client %s disconnected.\n", client->hostname);


	freerds_icp_DisconnectUserSession(connector->SessionId, &disconnected);
	client->Disconnect(client);
	CloseHandle(connector->hClientPipe);


	freerdp_peer_context_free(client);
	freerdp_peer_free(client);

	return NULL;
}

