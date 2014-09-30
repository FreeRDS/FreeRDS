/**
 * pbRPC - a simple, protocol buffer based RCP mechanism
 * Named pipe transport
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

#include <winpr/crt.h>
#include <winpr/pipe.h>

#include "pipe_transport.h"

struct np_transport_context
{
	pbRPCTransportContext context;
	HANDLE handle;
};
typedef struct np_transport_context NpTransportContext;

static int tp_npipe_open(pbRPCTransportContext* context, int timeout)
{
	HANDLE hNamedPipe = 0;
	char pipeName[] = "\\\\.\\pipe\\FreeRDS_SessionManager";
	NpTransportContext *np = (NpTransportContext *) context;

	if (!WaitNamedPipeA(pipeName, timeout))
	{
		return -1;
	}
	hNamedPipe = CreateFileA(pipeName,
			GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE))
	{
		return -1;
	}

	np->handle = hNamedPipe;

	return 0;
}

static int tp_npipe_close(pbRPCTransportContext* context)
{
	NpTransportContext* np = (NpTransportContext*) context;

	if(np->handle)
	{
		CloseHandle(np->handle);
		np->handle = 0;
	}

	return 0;
}

static int tp_npipe_write(pbRPCTransportContext* context, char* data, unsigned int datalen)
{
	DWORD bytesWritten;
	BOOL fSuccess = FALSE;
	NpTransportContext* np = (NpTransportContext*) context;

	fSuccess = WriteFile(np->handle, data, datalen, (LPDWORD) &bytesWritten, NULL);

	if (!fSuccess || (bytesWritten < datalen))
	{
		return -1;
	}

	return bytesWritten;
}

static int tp_npipe_read(pbRPCTransportContext* context, char* data, unsigned int datalen)
{
	NpTransportContext* np = (NpTransportContext*) context;
	DWORD bytesRead;
	BOOL fSuccess = FALSE;

	fSuccess = ReadFile(np->handle, data, datalen, &bytesRead, NULL);

	if (!fSuccess || (bytesRead < datalen))
	{
		return -1;
	}

	return bytesRead;
}

HANDLE tp_npipe_get_fds(pbRPCTransportContext* context)
{
	NpTransportContext *np = (NpTransportContext*)context;
	return np->handle;
}

pbRPCTransportContext* tp_npipe_new()
{
	NpTransportContext* np = malloc(sizeof(NpTransportContext));
	ZeroMemory(np, sizeof(NpTransportContext));
	pbRPCTransportContext *ctx = (pbRPCTransportContext*) np;
	ctx->open = tp_npipe_open;
	ctx->close = tp_npipe_close;
	ctx->read = tp_npipe_read;
	ctx->write  = tp_npipe_write;
	ctx->get_fds = tp_npipe_get_fds;
	return ctx;
}

void tp_npipe_free(pbRPCTransportContext* context)
{
	free(context);
}
