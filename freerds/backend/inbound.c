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

#include <freerds/backend.h>

#include "inbound.h"

rdsServerInterface* freerds_client_inbound_interface_new()
{
	rdsServerInterface* server = (rdsServerInterface*) malloc(sizeof(rdsServerInterface));

	if (server)
	{
		ZeroMemory(server, sizeof(rdsServerInterface));
	}

	return server;
}

rdsClientInterface* freerds_server_inbound_interface_new()
{
	rdsClientInterface* client = (rdsClientInterface*) malloc(sizeof(rdsClientInterface));

	if (client)
	{
		ZeroMemory(client, sizeof(rdsClientInterface));
	}

	return client;
}
