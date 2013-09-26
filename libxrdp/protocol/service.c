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

#include <xrdp-ng/xrdp.h>

#include <winpr/crt.h>

#include "service.h"

void* freerds_service_client_thread(void* arg)
{
	rdsService* service;

	service = (rdsService*) arg;

	return NULL;
}

void* freerds_service_listener_thread(void* arg)
{
	rdsService* service;

	service = (rdsService*) arg;

	while (1)
	{
		service->hClientPipe = freerds_named_pipe_accept(service->hServerPipe);

		if (!service->hClientPipe)
			break;

		if (service->Accept)
		{
			service->Accept(service);
		}
	}

	return NULL;
}

int freerds_service_start(rdsService* service)
{
	service->hServerPipe = freerds_named_pipe_create(service->SessionId, service->Endpoint);

	if (!service->hServerPipe)
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
	rdsService* service;

	service = (rdsService*) malloc(sizeof(rdsService));

	if (service)
	{
		ZeroMemory(service, sizeof(rdsService));

		service->SessionId = SessionId;
		service->Endpoint = _strdup(endpoint);
		service->ServerMode = TRUE;

		service->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		service->client = freerds_server_inbound_interface_new();
		service->server = freerds_server_outbound_interface_new();
	}

	return service;
}

void freerds_service_free(rdsService* service)
{
	if (service)
	{
		if (service->Endpoint)
			free(service->Endpoint);

		CloseHandle(service->StopEvent);

		free(service);
	}
}
