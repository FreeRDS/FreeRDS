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
 */

/**
 *
 * @file libscp_connection.c
 * @brief SCP_CONNECTION handling code
 * @author Simone Fedele
 *
 */

#include "libscp_connection.h"

#include <winpr/crt.h>

SCP_CONNECTION* scp_connection_create(int sck)
{
	SCP_CONNECTION* connection;

	connection = malloc(sizeof(SCP_CONNECTION));

	if (connection)
	{
		ZeroMemory(connection, sizeof(SCP_CONNECTION));

		connection->in_sck = sck;
		connection->in_s = Stream_New(NULL, 8196);
		connection->out_s = Stream_New(NULL, 8196);
	}

	return connection;
}

void scp_connection_destroy(SCP_CONNECTION* c)
{
	if (c)
	{
		Stream_Free(c->in_s, TRUE);
		Stream_Free(c->out_s, TRUE);
		free(c);
	}
}
