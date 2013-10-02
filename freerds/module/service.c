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

#include <freerds/freerds.h>

#include <winpr/crt.h>

#include "service.h"

void* freerds_service_client_thread(void* arg)
{
	rdsModule* module;
	rdsService* service;

	module = (rdsModule*) arg;
	service = (rdsService*) arg;

	if (module->hClientPipe)
	{
		while (WaitForSingleObject(module->hClientPipe, INFINITE) == WAIT_OBJECT_0)
		{
			if (freerds_transport_receive(module) < 0)
				break;
		}
	}

	return NULL;
}

void* freerds_service_listener_thread(void* arg)
{
	rdsModule* module;
	rdsService* service;

	module = (rdsModule*) arg;
	service = (rdsService*) arg;

	while (1)
	{
		module->hClientPipe = freerds_named_pipe_accept(module->hServerPipe);

		if (!module->hClientPipe)
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
	rdsModule* module;

	module = (rdsModule*) service;

	module->hServerPipe = freerds_named_pipe_create(module->SessionId, module->Endpoint);

	if (!module->hServerPipe)
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
	rdsModule* module;
	rdsService* service;

	service = (rdsService*) malloc(sizeof(rdsService));
	module = (rdsModule*) service;

	if (service)
	{
		ZeroMemory(service, sizeof(rdsService));

		module->SessionId = SessionId;
		module->Endpoint = _strdup(endpoint);
		module->ServerMode = TRUE;

		module->client = freerds_server_inbound_interface_new();
		module->server = freerds_server_outbound_interface_new();

		module->OutboundStream = Stream_New(NULL, 8192);
		module->InboundStream = Stream_New(NULL, 8192);

		module->InboundTotalLength = 0;
		module->InboundTotalCount = 0;

		module->OutboundTotalLength = 0;
		module->OutboundTotalCount = 0;

		service->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	return service;
}

void freerds_service_free(rdsService* service)
{
	rdsModule* module;

	module = (rdsModule*) service;

	if (service)
	{
		SetEvent(service->StopEvent);

		WaitForSingleObject(service->ServerThread, INFINITE);
		CloseHandle(service->ServerThread);

		Stream_Free(module->OutboundStream, TRUE);
		Stream_Free(module->InboundStream, TRUE);

		if (module->Endpoint)
			free(module->Endpoint);

		CloseHandle(service->StopEvent);

		free(service);
	}
}
