/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * Remote Procedure Call (RPC) System
 *
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include <freerds/rpc.h>

#include <winpr/crt.h>
#include <winpr/file.h>
#include <winpr/pipe.h>
#include <winpr/path.h>
#include <winpr/print.h>
#include <winpr/thread.h>

#define RDS_RPC_HEADER_LENGTH		4
#define RDS_RPC_PIPE_BUFFER_SIZE	0xFFFF

int freerds_rpc_named_pipe_read(HANDLE hNamedPipe, BYTE* data, DWORD length)
{
	BOOL fSuccess = FALSE;
	DWORD NumberOfBytesRead;
	DWORD TotalNumberOfBytesRead = 0;

	NumberOfBytesRead = 0;

	fSuccess = ReadFile(hNamedPipe, data, length, &NumberOfBytesRead, NULL);

	if (!fSuccess || (NumberOfBytesRead == 0))
	{
		return -1;
	}

	TotalNumberOfBytesRead += NumberOfBytesRead;
	length -= NumberOfBytesRead;
	data += NumberOfBytesRead;

	return TotalNumberOfBytesRead;
}

int freerds_rpc_named_pipe_write(HANDLE hNamedPipe, BYTE* data, DWORD length)
{
	BOOL fSuccess = FALSE;
	DWORD NumberOfBytesWritten;
	DWORD TotalNumberOfBytesWritten = 0;

	while (length > 0)
	{
		NumberOfBytesWritten = 0;

		fSuccess = WriteFile(hNamedPipe, data, length, &NumberOfBytesWritten, NULL);

		if (!fSuccess || (NumberOfBytesWritten == 0))
		{
			return -1;
		}

		TotalNumberOfBytesWritten += NumberOfBytesWritten;
		length -= NumberOfBytesWritten;
		data += NumberOfBytesWritten;
	}

	return TotalNumberOfBytesWritten;
}

void freerds_rpc_named_pipe_get_endpoint_name(const char* endpoint, char* dest, int length)
{
	sprintf_s(dest, length, "\\\\.\\pipe\\FreeRDS_%s", endpoint);
}

int freerds_rpc_named_pipe_clean(const char* pipeName)
{
	int status = 0;
	char* filename;

	filename = GetNamedPipeUnixDomainSocketFilePathA(pipeName);

	if (PathFileExistsA(filename))
	{
		DeleteFileA(filename);
		status = 1;
	}

	free(filename);
	return status;
}

HANDLE freerds_rpc_named_pipe_connect(const char* pipeName, DWORD nTimeOut)
{
	HANDLE hNamedPipe;

	if (!WaitNamedPipeA(pipeName, nTimeOut))
	{
		fprintf(stderr, "WaitNamedPipe failure: %s\n", pipeName);
		return NULL;
	}

	hNamedPipe = CreateFileA(pipeName,
			GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE))
	{
		fprintf(stderr, "Failed to create named pipe %s\n", pipeName);
		return NULL;
	}

	return hNamedPipe;
}

HANDLE freerds_rpc_named_pipe_connect_endpoint(const char* Endpoint, DWORD nTimeOut)
{
	char pipeName[256];
	freerds_rpc_named_pipe_get_endpoint_name(Endpoint, pipeName, sizeof(pipeName));
	return freerds_rpc_named_pipe_connect(pipeName, nTimeOut);
}

HANDLE freerds_rpc_named_pipe_create(const char* pipeName)
{
	HANDLE hNamedPipe;

	hNamedPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES, RDS_RPC_PIPE_BUFFER_SIZE,
			RDS_RPC_PIPE_BUFFER_SIZE, 0, NULL);

	if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE))
	{
		fprintf(stderr, "CreateNamedPipe failure\n");
		return NULL;
	}

	return hNamedPipe;
}

int freerds_rpc_transport_receive(rdsRpc* rpc)
{
	int status;
	wStream* s;
	UINT32 length;

	s = rpc->InboundStream;

	if (Stream_GetPosition(s) < RDS_RPC_HEADER_LENGTH)
	{
		status = freerds_rpc_named_pipe_read(rpc->hClientPipe, Stream_Pointer(s),
				RDS_RPC_HEADER_LENGTH - Stream_GetPosition(s));

		if (status < 0)
			return -1;

		if (status > 0)
			Stream_Seek(s, status);
	}

	if (Stream_GetPosition(s) >= RDS_RPC_HEADER_LENGTH)
	{
		length = *((UINT32*) Stream_Buffer(s));

		if (length - Stream_GetPosition(s))
		{
			status = freerds_rpc_named_pipe_read(rpc->hClientPipe, Stream_Pointer(s),
				length - Stream_GetPosition(s));

			if (status < 0)
				return -1;

			if (status > 0)
				Stream_Seek(s, status);
		}
	}

	if (Stream_GetPosition(s) >= RDS_RPC_HEADER_LENGTH)
	{
		length = *((UINT32*) Stream_Buffer(s));

		if (Stream_GetPosition(s) >= length)
		{
			Stream_SealLength(s);
			Stream_SetPosition(s, 4);

			if (rpc->ReceiveMessage)
			{
				status = rpc->ReceiveMessage(rpc, Stream_Pointer(s), length - 4);
			}
		}
	}

	return 0;
}

void* freerds_rpc_client_thread(void* arg)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[8];
	rdsRpc* rpc = (rdsRpc*) arg;

	nCount = 0;
	events[nCount++] = rpc->StopEvent;
	events[nCount++] = rpc->hClientPipe;

	while (1)
	{
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (status == WAIT_FAILED)
			break;

		if (WaitForSingleObject(rpc->StopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(rpc->hClientPipe, 0) == WAIT_OBJECT_0)
		{
			if (freerds_rpc_transport_receive(rpc) < 0)
				break;
		}
	}

	return NULL;
}

int freerds_rpc_client_start(rdsRpc* rpc)
{
	rpc->hClientPipe = freerds_rpc_named_pipe_connect(rpc->PipeName, 20);

	if (rpc->hClientPipe == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to create named pipe %s\n", rpc->PipeName);
		return -1;
	}

	fprintf(stderr, "Connected to endpoint: %s, named pipe: %s\n", rpc->Endpoint, rpc->PipeName);

	rpc->ServerThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) freerds_rpc_client_thread,
			(void*) rpc, CREATE_SUSPENDED, NULL);

	ResumeThread(rpc->ServerThread);

	return 0;
}

int freerds_rpc_client_stop(rdsRpc* rpc)
{
	if (rpc->StopEvent)
	{
		SetEvent(rpc->StopEvent);

		WaitForSingleObject(rpc->ServerThread, INFINITE);

		CloseHandle(rpc->StopEvent);
		rpc->StopEvent = NULL;
	}

	return 0;
}

int freerds_rpc_client_write_message(rdsRpc* rpc, BYTE* buffer, UINT32 length)
{
	int status;
	status = freerds_rpc_named_pipe_write(rpc->hClientPipe, buffer, length);
	return status;
}

HANDLE freerds_rpc_named_pipe_accept(HANDLE hServerPipe)
{
	BOOL fConnected;
	DWORD dwPipeMode;
	HANDLE hClientPipe;

	fConnected = ConnectNamedPipe(hServerPipe, NULL);

	if (!fConnected)
		fConnected = (GetLastError() == ERROR_PIPE_CONNECTED);

	if (!fConnected)
	{
		return NULL;
	}

	hClientPipe = hServerPipe;

	dwPipeMode = PIPE_NOWAIT;
	SetNamedPipeHandleState(hClientPipe, &dwPipeMode, NULL, NULL);

	return hClientPipe;
}

void* freerds_rpc_server_client_thread(void* arg)
{
	rdsRpc* rpc = (rdsRpc*) arg;

	if (rpc->hClientPipe)
	{
		while (WaitForSingleObject(rpc->hClientPipe, INFINITE) == WAIT_OBJECT_0)
		{
			if (freerds_rpc_transport_receive(rpc) < 0)
				break;
		}
	}

	return NULL;
}

void* freerds_rpc_server_listener_thread(void* arg)
{
	rdsRpc* rpc = (rdsRpc*) arg;

	while (1)
	{
		rpc->hClientPipe = freerds_rpc_named_pipe_accept(rpc->hServerPipe);

		if (!rpc->hClientPipe)
			break;

		if (rpc->Accept)
		{
			rpc->Accept(rpc);

			rpc->ClientThread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE) freerds_rpc_server_client_thread,
					(void*) rpc, 0, NULL);
		}
	}

	WaitForSingleObject(rpc->ClientThread, INFINITE);

	return NULL;
}

int freerds_rpc_server_start(rdsRpc* rpc)
{
	freerds_rpc_named_pipe_clean(rpc->PipeName);

	rpc->hServerPipe = freerds_rpc_named_pipe_create(rpc->PipeName);

	if (!rpc->hServerPipe)
		return -1;

	rpc->ServerThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) freerds_rpc_server_listener_thread,
			(void*) rpc, CREATE_SUSPENDED, NULL);

	ResumeThread(rpc->ServerThread);

	return 0;
}

int freerds_rpc_server_stop(rdsRpc* rpc)
{
	if (rpc->StopEvent)
	{
		SetEvent(rpc->StopEvent);

		WaitForSingleObject(rpc->ClientThread, 500);

		CloseHandle(rpc->StopEvent);
		rpc->StopEvent = NULL;
	}

	return 0;
}

int freerds_rpc_server_write_message(rdsRpc* rpc, BYTE* buffer, UINT32 length)
{
	int status;
	status = freerds_rpc_named_pipe_write(rpc->hClientPipe, buffer, length);
	return status;
}

rdsRpc* freerds_rpc_common_new(const char* Endpoint)
{
	int length;
	rdsRpc* rpc;

	rpc = (rdsRpc*) calloc(1, sizeof(rdsRpc));

	if (rpc)
	{
		rpc->ServerMode = TRUE;
		rpc->Endpoint = _strdup(Endpoint);

		rpc->OutboundStream = Stream_New(NULL, 8192);
		rpc->InboundStream = Stream_New(NULL, 8192);

		rpc->InboundTotalLength = 0;
		rpc->InboundTotalCount = 0;

		rpc->OutboundTotalLength = 0;
		rpc->OutboundTotalCount = 0;

		rpc->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		length = 64 + strlen(rpc->Endpoint);
		rpc->PipeName = (char*) malloc(length);
		sprintf_s(rpc->PipeName, length, "\\\\.\\pipe\\FreeRDS_%s", rpc->Endpoint);
	}

	return rpc;
}

void freerds_rpc_common_free(rdsRpc* rpc)
{
	if (rpc)
	{
		SetEvent(rpc->StopEvent);

		WaitForSingleObject(rpc->ServerThread, INFINITE);
		CloseHandle(rpc->ServerThread);

		Stream_Free(rpc->OutboundStream, TRUE);
		Stream_Free(rpc->InboundStream, TRUE);

		if (rpc->Endpoint)
			free(rpc->Endpoint);

		CloseHandle(rpc->StopEvent);

		free(rpc);
	}
}

rdsRpc* freerds_rpc_server_new(const char* Endpoint)
{
	rdsRpc* rpc;
	rpc = freerds_rpc_common_new(Endpoint);
	return rpc;
}

void freerds_rpc_server_free(rdsRpc* rpc)
{
	freerds_rpc_common_free(rpc);
}

rdsRpc* freerds_rpc_client_new(const char* Endpoint)
{
	rdsRpc* rpc;
	rpc = freerds_rpc_common_new(Endpoint);
	return rpc;
}

void freerds_rpc_client_free(rdsRpc* rpc)
{
	freerds_rpc_common_free(rpc);
}
