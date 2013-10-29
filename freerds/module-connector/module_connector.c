/**
 * FreeRDS module connector interface

 * Module connector is the glue between a service and rds
 *
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bmiklautz@thinstuff.at>
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

#include "xrdp.h"
#include <freerds/module_connector.h>
#include <winpr/wtypes.h>
#include <winpr/thread.h>
#include <winpr/synch.h>


rdsModuleConnector* freerds_module_connector_new(rdsConnection* connection)
{
	rdpSettings* settings;
	rdsModuleConnector* connector;

	settings = connection->settings;


	connector = (rdsModuleConnector*) malloc(sizeof(rdsModuleConnector));
	ZeroMemory(connector, sizeof(rdsModuleConnector));

	// htbd
	connector->SessionId = 10;

	connector->connection = connection;
	connector->settings = connection->settings;

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

void freerds_module_connector_free(rdsModuleConnector* connector)
{
	SetEvent(connector->StopEvent);

	WaitForSingleObject(connector->ServerThread, INFINITE);
	CloseHandle(connector->ServerThread);

	Stream_Free(connector->OutboundStream, TRUE);
	Stream_Free(connector->InboundStream, TRUE);

	CloseHandle(connector->StopEvent);
	CloseHandle(connector->hClientPipe);

	free(connector);
}
