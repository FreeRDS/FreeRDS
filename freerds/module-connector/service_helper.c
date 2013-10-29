/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng interprocess communication protocol
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

#include <freerds/service_helper.h>
#include <winpr/synch.h>

void* freerds_service_client_thread(void* arg)
{
	rdsModuleConnector* connector;
	rdsService* service;

	connector = (rdsModuleConnector*) arg;
	service = (rdsService*) arg;

	if (connector->hClientPipe)
	{
		while (WaitForSingleObject(connector->hClientPipe, INFINITE) == WAIT_OBJECT_0)
		{
			if (freerds_transport_receive(connector) < 0)
				break;
		}
	}

	return NULL;
}

void* freerds_service_listener_thread(void* arg)
{
	rdsModuleConnector* connector;
	rdsService* service;

	connector = (rdsModuleConnector*) arg;
	service = (rdsService*) arg;

	while (1)
	{
		connector->hClientPipe = freerds_named_pipe_accept(connector->hServerPipe);

		if (!connector->hClientPipe)
			break;

		if (service->Accept)
		{
			service->Accept(service);

			service->ClientThread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE) freerds_service_client_thread,
					(void*) service, 0, NULL);

			break;
		}
	}

	WaitForSingleObject(service->ClientThread, INFINITE);

	printf("Client Thread exited\n");

	return NULL;
}

int freerds_service_start(rdsService* service)
{
	rdsModuleConnector* connector;

	connector = (rdsModuleConnector*) service;

	connector->hServerPipe = freerds_named_pipe_create(connector->SessionId, connector->Endpoint);

	if (!connector->hServerPipe)
		return -1;

	service->ServerThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) freerds_service_listener_thread,
			(void*) service, CREATE_SUSPENDED, NULL);

	ResumeThread(service->ServerThread);

	return 0;
}

int freerds_service_stop(rdsService* service)
{
	SetEvent(service->StopEvent);

	return 0;
}

rdsService* freerds_service_new(DWORD SessionId, const char* endpoint)
{
	rdsModuleConnector* connector;
	rdsService* service;

	service = (rdsService*) malloc(sizeof(rdsService));
	connector = (rdsModuleConnector*) service;

	if (service)
	{
		ZeroMemory(service, sizeof(rdsService));

		connector->SessionId = SessionId;
		connector->Endpoint = _strdup(endpoint);
		connector->ServerMode = TRUE;

		connector->client = freerds_server_inbound_interface_new();
		connector->server = freerds_server_outbound_interface_new();

		connector->OutboundStream = Stream_New(NULL, 8192);
		connector->InboundStream = Stream_New(NULL, 8192);

		connector->InboundTotalLength = 0;
		connector->InboundTotalCount = 0;

		connector->OutboundTotalLength = 0;
		connector->OutboundTotalCount = 0;

		service->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	return service;
}


void freerds_service_free(rdsService* service)
{
	rdsModuleConnector* connector;

	connector = (rdsModuleConnector*) service;

	if (service)
	{
		SetEvent(service->StopEvent);

		WaitForSingleObject(service->ServerThread, INFINITE);
		CloseHandle(service->ServerThread);

		Stream_Free(connector->OutboundStream, TRUE);
		Stream_Free(connector->InboundStream, TRUE);

		if (connector->Endpoint)
			free(connector->Endpoint);

		CloseHandle(service->StopEvent);

		free(service);
	}
}
