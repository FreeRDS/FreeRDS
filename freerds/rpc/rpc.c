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

#define RDS_RPC_CONNECT_TIMEOUT		20
#define RDS_RPC_CONNECT_RETRIES		10
#define RDS_RPC_RECONNECT_INTERVAL	15000

/**
 *
 * Named Pipe Utility Functions used to create, connect, accept connections,
 * and read/write named pipes by both the client and server.
 */

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

HANDLE freerds_rpc_named_pipe_create(const char* pipeName)
{
	HANDLE hNamedPipe;

	hNamedPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES, RDS_RPC_PIPE_BUFFER_SIZE,
			RDS_RPC_PIPE_BUFFER_SIZE, 0, NULL);

	return hNamedPipe == INVALID_HANDLE_VALUE ? NULL : hNamedPipe;
}

HANDLE freerds_rpc_named_pipe_accept(const char* pipeName)
{
	BOOL fConnected;
	DWORD dwPipeMode;
	HANDLE hNamedPipe;

	/* Create an instance of the named pipe. */
	hNamedPipe = freerds_rpc_named_pipe_create(pipeName);

	if (hNamedPipe)
	{
		/* Wait for a client to connect to the named pipe. */
		fConnected = ConnectNamedPipe(hNamedPipe, NULL);

		if (!fConnected && (GetLastError() == ERROR_PIPE_CONNECTED))
		{
			fConnected = TRUE;
		}

		if (!fConnected)
		{
			return NULL;
		}

		dwPipeMode = PIPE_NOWAIT;
		SetNamedPipeHandleState(hNamedPipe, &dwPipeMode, NULL, NULL);
	}

	return hNamedPipe;
}

HANDLE freerds_rpc_named_pipe_connect(const char* pipeName)
{
	HANDLE hNamedPipe;
	int i;

	for (i = 0; i < RDS_RPC_CONNECT_RETRIES; i++)
	{
		/* Wait for an instance of the named pipe to become available. */
		if (!WaitNamedPipeA(pipeName, RDS_RPC_CONNECT_TIMEOUT)) continue;

		/* Attempt to connect to the named pipe. */
		hNamedPipe = CreateFileA(pipeName,
			GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);

		/* Failure to connect means someone else claimed the instance of the named pipe. */
		if (hNamedPipe == INVALID_HANDLE_VALUE) continue;

		printf("freerds_rpc_named_pipe_connect: connected to %s\n", pipeName);

		return hNamedPipe;
	}

	return NULL;
}

HANDLE freerds_rpc_named_pipe_connect_endpoint(const char* Endpoint)
{
	char pipeName[256];

	freerds_rpc_named_pipe_get_endpoint_name(Endpoint, pipeName, sizeof(pipeName));

	return freerds_rpc_named_pipe_connect(pipeName);
}

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



/**
 *
 * RPC common functions used by both client and server
 *
 */

int freerds_rpc_transport_receive(rdsRpcClient* rpcClient)
{
	int status;
	wStream* s;
	UINT32 length;

	s = rpcClient->InboundStream;

	if (Stream_GetPosition(s) < RDS_RPC_HEADER_LENGTH)
	{
		status = freerds_rpc_named_pipe_read(rpcClient->hClientPipe, Stream_Pointer(s),
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
			status = freerds_rpc_named_pipe_read(rpcClient->hClientPipe, Stream_Pointer(s),
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
			//Stream_SealLength(s);
			//Stream_SetPosition(s, 4);

			Stream_SetPosition(s, 0);

			if (rpcClient->MessageReceived)
			{
				status = rpcClient->MessageReceived(rpcClient, Stream_Pointer(s) + 4, length - 4);
			}
		}
	}

	return 0;
}




/**
 *
 * RPC client implementation
 *
 */

rdsRpcClient* freerds_rpc_client_new(const char* Endpoint)
{
	rdsRpcClient* rpcClient;

	rpcClient = (rdsRpcClient*) malloc(sizeof(rdsRpcClient));
	if (rpcClient)
	{
		int length;

		ZeroMemory(rpcClient, sizeof(rdsRpcClient));

		rpcClient->Endpoint = _strdup(Endpoint);

		length = 64 + strlen(rpcClient->Endpoint);
		rpcClient->PipeName = (char*)malloc(length);
		sprintf_s(rpcClient->PipeName, length, "\\\\.\\pipe\\FreeRDS_%s", rpcClient->Endpoint);

		rpcClient->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		rpcClient->InboundStream = Stream_New(NULL, 8192);
	}

	return rpcClient;
}

void freerds_rpc_client_free(rdsRpcClient* rpcClient)
{
	freerds_rpc_client_stop(rpcClient);

	CloseHandle(rpcClient->hStopEvent);

	if (rpcClient->Endpoint)
	{
		free(rpcClient->Endpoint);
	}

	if (rpcClient->PipeName)
	{
		free(rpcClient->PipeName);
	}

	if (rpcClient->InboundStream)
	{
		Stream_Free(rpcClient->InboundStream, TRUE);
	}

	free(rpcClient);
}

void* freerds_rpc_client_thread(void* arg)
{
	rdsRpcClient* rpcClient = (rdsRpcClient*) arg;

	BOOL bRunning = TRUE;

	while (bRunning)
	{
		if (rpcClient->hClientPipe == NULL)
		{
			switch (WaitForSingleObject(rpcClient->hStopEvent, RDS_RPC_RECONNECT_INTERVAL))
			{
				case WAIT_OBJECT_0:
					bRunning = FALSE;
					break;

				case WAIT_TIMEOUT:
					/* Attempt a reconnection to the server. */
					rpcClient->hClientPipe = freerds_rpc_named_pipe_connect(rpcClient->PipeName);
					break;

				default:
					bRunning = FALSE;
					break;
			}
		}
		else
		{
			DWORD nCount;
			HANDLE hObjects[2];

			nCount = 0;
			hObjects[nCount++] = rpcClient->hStopEvent;
			hObjects[nCount++] = rpcClient->hClientPipe;

			switch (WaitForMultipleObjects(nCount, hObjects, FALSE, INFINITE))
			{
				case WAIT_OBJECT_0:
					bRunning = FALSE;
					break;

				case WAIT_OBJECT_0 + 1:
					if (freerds_rpc_transport_receive(rpcClient) < 0)
					{
						/* The connection to the peer has been closed. */
						CloseHandle(rpcClient->hClientPipe);
						rpcClient->hClientPipe = NULL;

						/* Invoke the ConnectionClosed callback function. */
						if (rpcClient->ConnectionClosed)
						{
							rpcClient->ConnectionClosed(rpcClient);
						}

						/* If we're operating as the server, then terminate the thread. */
						if (rpcClient->RpcServer)
						{
							bRunning = FALSE;
						}
					}
					break;

				default:
					bRunning = FALSE;
					break;
			}
		}
	}

	/* If we're operating as the server, clean up the thread. */
	if (rpcClient->RpcServer)
	{
		ArrayList_Remove(rpcClient->RpcServer->ClientList, rpcClient);

		if (rpcClient->hClientPipe)
		{
			CloseHandle(rpcClient->hClientPipe);
			rpcClient->hClientPipe = NULL;
		}

		CloseHandle(rpcClient->hClientThread);
		rpcClient->hClientThread = NULL;

		freerds_rpc_client_free(rpcClient);
	}

	return NULL;
}

int freerds_rpc_client_start(rdsRpcClient* rpcClient)
{
	rpcClient->hClientPipe = freerds_rpc_named_pipe_connect(rpcClient->PipeName);

	if (rpcClient->hClientPipe == NULL)
	{
		fprintf(stderr, "Failed to create named pipe %s\n", rpcClient->PipeName);
		return -1;
	}

	fprintf(stderr, "Connected to endpoint: %s, named pipe: %s\n", rpcClient->Endpoint, rpcClient->PipeName);

	ResetEvent(rpcClient->hStopEvent);

	rpcClient->hClientThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) freerds_rpc_client_thread,
			(void*) rpcClient, 0, NULL);

	if (rpcClient->hClientThread == NULL)
	{
		fprintf(stderr, "Failed to create client thread\n");
		CloseHandle(rpcClient->hClientPipe);
		rpcClient->hClientPipe = NULL;
		return -1;
	}

	return 0;
}

int freerds_rpc_client_stop(rdsRpcClient* rpcClient)
{
	if (rpcClient->hClientThread)
	{
		SetEvent(rpcClient->hStopEvent);

		WaitForSingleObject(rpcClient->hClientThread, INFINITE);
		CloseHandle(rpcClient->hClientThread);
		rpcClient->hClientThread = NULL;

		CloseHandle(rpcClient->hClientPipe);
		rpcClient->hClientPipe = NULL;
	}

	return 0;
}

int freerds_rpc_client_send_message(rdsRpcClient* rpcClient, BYTE* buffer, UINT32 length)
{
	int totalLength;
	int status;

	totalLength = length + sizeof(UINT32);

	status = freerds_rpc_named_pipe_write(rpcClient->hClientPipe, (BYTE*) &totalLength, sizeof(UINT32));
	if (status != sizeof(UINT32))
		return -1;

	status = freerds_rpc_named_pipe_write(rpcClient->hClientPipe, buffer, length);

	return status;
}




/**
 *
 * RPC server implementation
 *
 */

rdsRpcServer* freerds_rpc_server_new(const char* Endpoint)
{
	rdsRpcServer* rpcServer;

	rpcServer = (rdsRpcServer*) malloc(sizeof(rdsRpcServer));
	if (rpcServer)
	{
		int length;

		ZeroMemory(rpcServer, sizeof(rdsRpcServer));

		rpcServer->Endpoint = _strdup(Endpoint);

		length = 64 + strlen(rpcServer->Endpoint);
		rpcServer->PipeName = (char*)malloc(length);
		sprintf_s(rpcServer->PipeName, length, "\\\\.\\pipe\\FreeRDS_%s", rpcServer->Endpoint);

		rpcServer->ClientList = ArrayList_New(TRUE);

		rpcServer->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	return rpcServer;
}

void freerds_rpc_server_free(rdsRpcServer* rpcServer)
{
	freerds_rpc_server_stop(rpcServer);

	CloseHandle(rpcServer->hStopEvent);

	ArrayList_Free(rpcServer->ClientList);

	if (rpcServer->Endpoint)
	{
		free(rpcServer->Endpoint);
	}

	if (rpcServer->PipeName)
	{
		free(rpcServer->PipeName);
	}

	free(rpcServer);
}

void* freerds_rpc_server_thread(void* arg)
{
	rdsRpcServer* rpcServer = (rdsRpcServer*) arg;

	int count;
	int i;

	while (1)
	{
		rdsRpcClient* rpcClient;
		HANDLE hClientPipe;

		/* Accept a connection from the client. */
		hClientPipe = freerds_rpc_named_pipe_accept(rpcServer->PipeName);
		if (hClientPipe == NULL) break;

		/* If the stop event was signalled... */
		if (WaitForSingleObject(rpcServer->hStopEvent, 0) == WAIT_OBJECT_0)
		{
			CloseHandle(hClientPipe);
			break;
		}

		/* Create an RPC client instance. */
		rpcClient = freerds_rpc_client_new(rpcServer->Endpoint);
		if (rpcClient == NULL) break;

		rpcClient->RpcServer = rpcServer;

		rpcClient->hClientPipe = hClientPipe;

		rpcClient->ConnectionClosed = rpcServer->ConnectionClosed;
		rpcClient->MessageReceived = rpcServer->MessageReceived;

		ArrayList_Add(rpcServer->ClientList, rpcClient);

		/* Create the client thread. */
		rpcClient->hClientThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) freerds_rpc_client_thread,
			(void*) rpcClient, 0, NULL);

		/* Invoke the ConnectionAccepted callback function. */
		if (rpcServer->ConnectionAccepted)
		{
			rpcServer->ConnectionAccepted(rpcClient);
		}
	}

	ArrayList_Lock(rpcServer->ClientList);

	/* Signal all client threads to stop. */
	count = ArrayList_Count(rpcServer->ClientList);
	for (i = 0; i < count; i++)
	{
		rdsRpcClient* rpcClient;

		rpcClient = (rdsRpcClient*) ArrayList_GetItem(rpcServer->ClientList, i);

		SetEvent(rpcClient->hStopEvent);
	}

	/* Wait for remaining client threads to gracefully terminate. */
	for (i = 0; i < 30; i++)
	{
		if (ArrayList_Count(rpcServer->ClientList) == 0) break;

		ArrayList_Unlock(rpcServer->ClientList);
		Sleep(1000);
		ArrayList_Lock(rpcServer->ClientList);
	}

	ArrayList_Unlock(rpcServer->ClientList);

	return NULL;
}

int freerds_rpc_server_start(rdsRpcServer* rpcServer)
{
	if (rpcServer->hServerThread)
		return -1;

	freerds_rpc_named_pipe_clean(rpcServer->PipeName);

	ResetEvent(rpcServer->hStopEvent);

	rpcServer->hServerThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) freerds_rpc_server_thread,
			(void*) rpcServer, 0, NULL);

	return 0;
}

int freerds_rpc_server_stop(rdsRpcServer* rpcServer)
{
	HANDLE hNamedPipe;

	if (rpcServer->hServerThread)
	{
		SetEvent(rpcServer->hStopEvent);

		hNamedPipe = freerds_rpc_named_pipe_connect(rpcServer->PipeName);
		if (hNamedPipe)
		{
			CloseHandle(hNamedPipe);
		}

		WaitForSingleObject(rpcServer->hServerThread, INFINITE);
		CloseHandle(rpcServer->hServerThread);
		rpcServer->hServerThread = NULL;
	}

	return 0;
}

int freerds_rpc_server_broadcast_message(rdsRpcServer* rpcServer, BYTE* buffer, UINT32 length)
{
	int status;
	int index;

	ArrayList_Lock(rpcServer->ClientList);
	for (index = 0; index < ArrayList_Count(rpcServer->ClientList); index++)
	{
		rdsRpcClient* rpcClient;

		rpcClient = (rdsRpcClient*) ArrayList_GetItem(rpcServer->ClientList, index);
		status = freerds_rpc_client_send_message(rpcClient, buffer, length);
	}
	ArrayList_Unlock(rpcServer->ClientList);

	return 0;
}
