/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * Backend Interface
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bernhard.miklautz@thincast.com>
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
#include <freerds/backend.h>

#ifndef _WIN32
#include <sys/shm.h>
#include <sys/types.h>
#endif

rdsBackendConnector* freerds_connector_new(rdsConnection* connection)
{
	rdpSettings* settings;
	rdsBackendConnector* connector;

	settings = (connection) ? connection->settings : NULL;

	connector = (rdsBackendConnector*) calloc(1, sizeof(rdsBackendConnector));

	connector->connection = connection;
	connector->settings = settings;

	connector->client = freerds_client_outbound_interface_new();
	connector->server = freerds_server_outbound_interface_new();

	connector->OutboundStream = Stream_New(NULL, 8192);
	connector->InboundStream = Stream_New(NULL, 8192);

	connector->InboundTotalLength = 0;
	connector->InboundTotalCount = 0;

	connector->OutboundTotalLength = 0;
	connector->OutboundTotalCount = 0;

	connector->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	return connector;
}

void freerds_connector_free(rdsBackendConnector* connector)
{
	SetEvent(connector->StopEvent);

	WaitForSingleObject(connector->ServerThread, INFINITE);
	CloseHandle(connector->ServerThread);

	Stream_Free(connector->OutboundStream, TRUE);
	Stream_Free(connector->InboundStream, TRUE);

	CloseHandle(connector->StopEvent);
	CloseHandle(connector->hClientPipe);

	if (connector->ServerList)
		LinkedList_Free(connector->ServerList);

	if (connector->ServerQueue)
		MessageQueue_Free(connector->ServerQueue);

	if (connector->Endpoint)
		free(connector->Endpoint);

	if (connector->framebuffer.fbAttached)
	{
#ifndef _WIN32
		shmdt(connector->framebuffer.fbSharedMemory);

		fprintf(stderr, "connector_free: detached shm %d from %p\n",
				connector->framebuffer.fbSegmentId, connector->framebuffer.fbSharedMemory);
#endif
	}

	free(connector);
}

BOOL freerds_connector_connect(rdsBackendConnector* connector)
{
	HANDLE hClientPipe;

	hClientPipe = freerds_named_pipe_connect(connector->Endpoint, 20);

	if (hClientPipe == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to create named pipe %s\n", connector->Endpoint);
		return FALSE;
	}

	printf("Connected to endpoint %s\n", connector->Endpoint);

	if (freerds_init_client(hClientPipe, connector->connection->settings, connector->OutboundStream) < 0)
	{
		fprintf(stderr, "Error sending initial packet with %s\n", connector->Endpoint);
		return FALSE;
	}

	connector->hClientPipe = hClientPipe;
	connector->GetEventHandles = freerds_client_get_event_handles;
	connector->CheckEventHandles = freerds_client_check_event_handles;

	connector->ServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) freerds_client_thread,
			(void*) connector, CREATE_SUSPENDED, NULL);

	freerds_client_inbound_connector_init(connector);

	ResumeThread(connector->ServerThread);

	return TRUE;
}
