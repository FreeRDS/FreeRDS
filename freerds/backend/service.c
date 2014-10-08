/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013-2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include "freerds.h"
#include <winpr/wtypes.h>
#include <winpr/thread.h>
#include <winpr/synch.h>


void* freerds_service_client_thread(void* arg)
{
	rdsBackendService* service;

	service = (rdsBackendService*) arg;

	if (service->hClientPipe)
	{
		while (WaitForSingleObject(service->hClientPipe, INFINITE) == WAIT_OBJECT_0)
		{
			if (freerds_transport_receive((rdsBackend *)service) < 0)
				break;
		}
	}

	return NULL;
}

void* freerds_service_listener_thread(void* arg)
{
	rdsBackendService* service = (rdsBackendService *)arg;

	while (1)
	{
		service->hClientPipe = freerds_named_pipe_accept((rdsBackend *)(service)->hServerPipe);

		if (!service->hClientPipe)
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

int freerds_service_start(rdsBackendService* service)
{
	service->hServerPipe = freerds_named_pipe_create_endpoint(service->SessionId, service->Endpoint);

	if (!service->hServerPipe)
		return -1;

	service->ServerThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) freerds_service_listener_thread,
			(void*) service, CREATE_SUSPENDED, NULL);

	ResumeThread(service->ServerThread);

	return 0;
}

int freerds_service_stop(rdsBackendService* service)
{
	SetEvent(((rdsBackend *)service)->StopEvent);

	return 0;
}

rdsBackendService* freerds_service_new(DWORD SessionId, const char* endpoint)
{
	rdsBackendService* service;

	service = (rdsBackendService*) malloc(sizeof(rdsBackendService));

	if (service)
	{
		rdsBackend* backend = (rdsBackend*) service;
		ZeroMemory(service, sizeof(rdsBackendService));

		backend->Endpoint = _strdup(endpoint);
		backend->ServerMode = TRUE;

		backend->client = freerds_server_inbound_interface_new();
		backend->server = freerds_server_outbound_interface_new();

		backend->OutboundStream = Stream_New(NULL, 8192);
		backend->InboundStream = Stream_New(NULL, 8192);

		backend->InboundTotalLength = 0;
		backend->InboundTotalCount = 0;

		backend->OutboundTotalLength = 0;
		backend->OutboundTotalCount = 0;

		backend->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		service->SessionId = SessionId;
	}

	return service;
}

void freerds_service_free(rdsBackendService* service)
{
	rdsBackend* backend;

	backend = (rdsBackend*) service;

	if (service)
	{
		SetEvent(backend->StopEvent);

		WaitForSingleObject(backend->ServerThread, INFINITE);
		CloseHandle(backend->ServerThread);

		Stream_Free(backend->OutboundStream, TRUE);
		Stream_Free(backend->InboundStream, TRUE);

		if (backend->Endpoint)
			free(backend->Endpoint);

		CloseHandle(backend->StopEvent);

		free(service);
	}
}
