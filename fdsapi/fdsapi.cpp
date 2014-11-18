/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * FDSAPI WTSAPI Stubs
 *
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
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

#include "fdsapi.h"

#include <winpr/crt.h>
#include <winpr/wnd.h>
#include <winpr/wlog.h>
#include <winpr/pipe.h>
#include <winpr/file.h>
#include <winpr/print.h>
#include <winpr/sysinfo.h>
#include <winpr/wtsapi.h>
#include <winpr/winsock.h>

#include <freerds/rpc.h>

#include "FDSApiMessages.h"

static DWORD g_currentSessionId = 0xFFFFFFFF;

static rdsRpcClient* g_RpcClient = NULL;

static wLog* g_logger = NULL;

struct _FDSAPI_CHANNEL
{
	char* guid;
	UINT32 port;
	SOCKET socket;
	HANDLE event;
};
typedef struct _FDSAPI_CHANNEL FDSAPI_CHANNEL;

static FDSAPI_CHANNEL* FDSAPI_Channel_New()
{
	FDSAPI_CHANNEL* pChannel;

	pChannel = (FDSAPI_CHANNEL*) calloc(1, sizeof(FDSAPI_CHANNEL));

	if (pChannel)
	{

	}

	return pChannel;
}

static int FDSAPI_Channel_Connect(FDSAPI_CHANNEL* pChannel, const char* guid, UINT32 port)
{
	int status;
	int optlen = 0;
	UINT32 optval = 0;
	unsigned long addr;
	struct sockaddr_in sockAddr;

	if (strlen(guid) != 36)
		return -1001;

	pChannel->port = port;

	pChannel->guid = _strdup(guid);

	if (!pChannel->guid)
		return -1002;

	pChannel->socket = _socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (pChannel->socket == INVALID_SOCKET)
		return -1003;

	addr = _inet_addr("127.0.0.1");

	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = addr;
	sockAddr.sin_port = htons(port);

	status = _connect(pChannel->socket, (struct sockaddr*) &sockAddr, sizeof(sockAddr));

	if (status != 0)
	{
		closesocket(pChannel->socket);
		pChannel->socket = 0;
		return -1004;
	}

	status = _send(pChannel->socket, guid, 36, 0);

	if (status != 36)
	{
		closesocket(pChannel->socket);
		pChannel->socket = 0;
		return -1005;
	}

	optval = TRUE;
	optlen = sizeof(optval);

	_setsockopt(pChannel->socket, IPPROTO_TCP, TCP_NODELAY, (char*) &optval, optlen);

	pChannel->event = CreateFileDescriptorEvent(NULL, FALSE, FALSE, (int) pChannel->socket);

	return 1;
}

static void FDSAPI_Channel_Free(FDSAPI_CHANNEL* pChannel)
{
	if (!pChannel)
		return;

	if (pChannel->socket)
	{
		closesocket(pChannel->socket);
		pChannel->socket = 0;
	}

	if (pChannel->event)
	{
		CloseHandle(pChannel->event);
		pChannel->event = NULL;
	}

	free(pChannel->guid);

	free(pChannel);
}

/*
 * Most calls result in a request/response message being sent and
 * received from the FreeRDS server.  The following code handles
 * the blocking nature of these calls in a thread-safe manner.
 * Each request is tagged with a unique request id and a response
 * is paired with the request.
 */

typedef struct
{
	HANDLE hWaitEvent;
	FDSAPI_MESSAGE* requestMsg;
	FDSAPI_MESSAGE* responseMsg;
}
FDSAPI_REQUEST_ITEM;

static UINT16 g_nextRequestId;
static wArrayList* g_activeRequestList;

static BOOL FDSAPI_SendRequest(FDSAPI_MESSAGE* requestMsg, FDSAPI_MESSAGE* responseMsg)
{
	FDSAPI_REQUEST_ITEM requestItem;
	BOOL success = FALSE;
	wStream* s = NULL;
	int status;

	ZeroMemory(&requestItem, sizeof(requestItem));

	/* Enter critical section. */
	ArrayList_Lock(g_activeRequestList);

	/* Set the request id. */
	requestMsg->requestId = g_nextRequestId++;

	/* Leave critical section. */
	ArrayList_Unlock(g_activeRequestList);

	WLog_Print(g_logger, WLOG_DEBUG, "sending request %d", requestMsg->requestId);

	/* Encode the request. */
	s = FDSAPI_EncodeMessage(requestMsg);

	if (!s)
	{
		WLog_Print(g_logger, WLOG_ERROR, "failed to encode request");
		return FALSE;
	}

	/* Add the request to the list. */
	requestItem.hWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!requestItem.hWaitEvent)
	{
		WLog_Print(g_logger, WLOG_ERROR, "failed to create event");
		goto EXCEPTION;
	}

	requestItem.requestMsg = requestMsg;
	requestItem.responseMsg = responseMsg;

	ArrayList_Add(g_activeRequestList, &requestItem);

	/* Send the request to the FreeRDS server. */
	status = freerds_rpc_client_send_message(g_RpcClient, Stream_Buffer(s), Stream_Length(s));

	if (status == Stream_Length(s))
	{
		WLog_Print(g_logger, WLOG_DEBUG, "waiting for response");

		/* Wait for the response. */
		WaitForSingleObject(requestItem.hWaitEvent, INFINITE);

		WLog_Print(g_logger, WLOG_DEBUG, "received response %d", responseMsg->requestId);

		/* Check for the expected response. */
		if (requestMsg->requestId == responseMsg->requestId)
		{
			success = TRUE;
		}
	}
	else
	{
		WLog_Print(g_logger, WLOG_ERROR, "failed to send request");
	}

	/* Remove the request from the list. */	
	ArrayList_Remove(g_activeRequestList, &requestItem);

	CloseHandle(requestItem.hWaitEvent);

EXCEPTION:
	if (s)
	{
		Stream_Free(s, TRUE);
	}

	return success;
}

static int FDSAPI_HandleResponse(rdsRpcClient* rpcClient, FDSAPI_MESSAGE* msg)
{
	int index;
	FDSAPI_REQUEST_ITEM* requestItem;

	/* Find the matching request. */
	ArrayList_Lock(g_activeRequestList);

	for (index = 0; index < ArrayList_Count(g_activeRequestList); index++)
	{
		requestItem = (FDSAPI_REQUEST_ITEM*) ArrayList_GetItem(g_activeRequestList, index);

		if (msg->requestId == requestItem->requestMsg->requestId)
		{
			/* Wake up the requesting thread. */
			CopyMemory(requestItem->responseMsg, msg, sizeof(FDSAPI_MESSAGE));
			SetEvent(requestItem->hWaitEvent);
			break;
		}
	}

	ArrayList_Unlock(g_activeRequestList);

	return 0;
}


/**
 * As calls to WTSWaitSystemEvent and WTSRegisterSessionNotification
 * are made from various threads in the process, a list is maintained
 * of FDSAPI_SESSION_EVENT_OBSERVER structures.  Each time a session
 * event message (FDSAPI_SESSION_EVENT) is received from the FreeRDS
 * server, we need to walk through this list and deliver the session
 * event in the desired manner (either by completing the wait being
 * done in WTSWaitSystemEvent or sending a message to the HWND that
 * was registered via WTSRegisterSessionNotification).
 */

#define FDSAPI_WAIT_SYSTEM_EVENT_TYPE		1
#define FDSAPI_SESSION_NOTIFICATION_TYPE	2

typedef struct
{
	HANDLE hWaitEvent;
	DWORD dwEventMask;
	DWORD dwEventFlags;
}
FDSAPI_WAIT_SYSTEM_EVENT;

typedef struct
{
	HWND hWnd;
	DWORD dwFlags;
}
FDSAPI_SESSION_NOTIFICATION;

typedef struct
{
	int type;
	union
	{
		FDSAPI_WAIT_SYSTEM_EVENT waitSystemEvent;
		FDSAPI_SESSION_NOTIFICATION sessionNotification;
	} u;
}
FDSAPI_SESSION_EVENT_OBSERVER;

static wArrayList* g_sessionEventObserverList;
static DWORD g_sessionEventTimeDelay = 1000;
static DWORD g_sessionEventTimeStamp;

static int FDSAPI_HandleSessionEvent(rdsRpcClient* rpcClient, FDSAPI_SESSION_EVENT* sessionEvent)
{
	int index;
	DWORD sleepTime;
	FDSAPI_SESSION_EVENT_OBSERVER* observer;

	/* Enter critical section. */
	ArrayList_Lock(g_sessionEventObserverList);

	/*
	 * Wait a short period of time to allow session event observers to
	 * register.  This can happen if a WTSWaitSystemEvent gets satisfied
	 * and another session event comes in immediately.  In this case, the
	 * possibility exists that a session event could get missed.
	 */
	sleepTime = g_sessionEventTimeDelay - (GetTickCount() - g_sessionEventTimeStamp);

	if (sleepTime < g_sessionEventTimeDelay)
	{
		ArrayList_Unlock(g_sessionEventObserverList);
		Sleep(sleepTime);
		ArrayList_Lock(g_sessionEventObserverList);
	}

	/* Loop through all items in the list... */
	for (index = 0; index < ArrayList_Count(g_sessionEventObserverList); index++)
	{
		observer = (FDSAPI_SESSION_EVENT_OBSERVER*) ArrayList_GetItem(g_sessionEventObserverList, index);

		switch (observer->type)
		{
			case FDSAPI_WAIT_SYSTEM_EVENT_TYPE:
			{
				DWORD dwEventFlags = 0;

				/* Map the state change to event flags. */
				switch (sessionEvent->stateChange)
				{
					case WTS_SESSION_CREATE:
						dwEventFlags = WTS_EVENT_CREATE;
						break;

					case WTS_SESSION_TERMINATE:
						dwEventFlags = WTS_EVENT_DELETE;
						break;

					case WTS_CONSOLE_CONNECT:
					case WTS_REMOTE_CONNECT:
						dwEventFlags = WTS_EVENT_STATECHANGE | WTS_EVENT_CONNECT;
						break;

					case WTS_CONSOLE_DISCONNECT:
					case WTS_REMOTE_DISCONNECT:
						dwEventFlags = WTS_EVENT_STATECHANGE | WTS_EVENT_DISCONNECT;
						break;

					case WTS_SESSION_LOGON:
						dwEventFlags = WTS_EVENT_STATECHANGE | WTS_EVENT_LOGON;
						break;

					case WTS_SESSION_LOGOFF:
						dwEventFlags = WTS_EVENT_STATECHANGE | WTS_EVENT_LOGOFF;
						break;

					default:
						break;
				}

				/* If the event flags match the event mask... */
				if ((dwEventFlags & observer->u.waitSystemEvent.dwEventMask) != 0)
				{
					/* Save the event flags. */
					observer->u.waitSystemEvent.dwEventFlags |= dwEventFlags;

					/* Wake up the thread calling WTSWaitSystemEvent. */
					SetEvent(observer->u.waitSystemEvent.hWaitEvent);
				}

				break;
			}

			case FDSAPI_SESSION_NOTIFICATION_TYPE:
			{
				/* If the session matches a session being monitored... */
				if ((observer->u.sessionNotification.dwFlags == NOTIFY_FOR_ALL_SESSIONS) ||
				    ((observer->u.sessionNotification.dwFlags == NOTIFY_FOR_THIS_SESSION)
				        && (sessionEvent->sessionId == g_currentSessionId)))
				{
					/* Send a WM_WTSSESSION_CHANGE message to the window. */
					SendMessage(observer->u.sessionNotification.hWnd,
						WM_WTSSESSION_CHANGE,
						(WPARAM) sessionEvent->stateChange,
						(LPARAM) sessionEvent->sessionId);
				}

				break;
			}
		}
	}

	/* Update the timestamp. */
	g_sessionEventTimeStamp = GetTickCount();

	/* Leave critical section. */
	ArrayList_Unlock(g_sessionEventObserverList);

	return 0;
}

int FDSAPI_RpcConnectionClosed(rdsRpcClient* rpcClient)
{
	WLog_Print(g_logger, WLOG_DEBUG, "connection closed");

	return 0;
}

int FDSAPI_RpcMessageReceived(rdsRpcClient* rpcClient, BYTE* buffer, UINT32 length)
{
	int status;
	wStream *s;
	FDSAPI_MESSAGE msg;

	status = -1;

	WLog_Print(g_logger, WLOG_DEBUG, "message received");

	/* Allocate a stream. */
	s = Stream_New(buffer, length);

	if (!s)
	{
		WLog_Print(g_logger, WLOG_ERROR, "failed to allocate stream");
		return -1;
	}

	/* Decode the message. */
	if (FDSAPI_DecodeMessage(s, &msg))
	{
		switch (msg.messageId)
		{
			case FDSAPI_SESSION_EVENT_ID:
				status = FDSAPI_HandleSessionEvent(rpcClient, &msg.u.sessionEvent);
				break;

			default:
				status = FDSAPI_HandleResponse(rpcClient, &msg);
				break;
		}
	}
	else
	{
		WLog_Print(g_logger, WLOG_ERROR, "failed to decode message");
	}

	/* Free the stream. */
	Stream_Free(s, FALSE);

	return status;
}

static BOOL ConnectClient()
{
	if (!g_logger)
	{
		g_logger = WLog_Get("freerds.fdsapi");
	}

	if (!g_RpcClient)
	{
		WLog_Print(g_logger, WLOG_DEBUG, "connecting to FDSAPI server");

		g_activeRequestList = ArrayList_New(TRUE);
		g_sessionEventObserverList = ArrayList_New(TRUE);

		g_RpcClient = freerds_rpc_client_new("FDSAPI");
		g_RpcClient->ConnectionClosed = FDSAPI_RpcConnectionClosed;
		g_RpcClient->MessageReceived = FDSAPI_RpcMessageReceived;

		freerds_rpc_client_start(g_RpcClient);
	}

	return TRUE;
}

/**
 * Local Utility Functions
 */

static BOOL GetCurrentSessionId(LPDWORD pSessionId)
{
	char* env;

	/* If the session has been previously obtained... */
	if (g_currentSessionId != 0xFFFFFFFF)
	{
		/* Simply return it to the caller. */
		*pSessionId = g_currentSessionId;
		return TRUE;
	}

	/* The FREERDS_SID environment variable has the session id. */
	env = getenv("FREERDS_SID");

	if (env)
	{
		g_currentSessionId = atoi(env);
		*pSessionId = g_currentSessionId;
		return TRUE;
	}

	return FALSE;
}

static BOOL CheckSessionId(LPDWORD pSessionId)
{
	if (*pSessionId == WTS_CURRENT_SESSION)
	{
		return GetCurrentSessionId(pSessionId);
	}

	/**
	 * TODO: Need to do some additional checks to prevent
	 * normal users from being able to interact with other
	 * sessions.
	 */
	return TRUE;
}


/**
 * WTSAPI Public Interface
 */

BOOL WINAPI
FreeRDS_WTSStartRemoteControlSessionW(
	LPWSTR pTargetServerName,
	ULONG TargetLogonId,
	BYTE HotkeyVk,
	USHORT HotkeyModifiers
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSStartRemoteControlSessionA(
	LPSTR pTargetServerName,
	ULONG TargetLogonId,
	BYTE HotkeyVk,
	USHORT HotkeyModifiers
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSStopRemoteControlSession(
	ULONG LogonId
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSConnectSessionW(
	ULONG LogonId,
	ULONG TargetLogonId,
	PWSTR pPassword,
	BOOL bWait
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSConnectSessionA(
	ULONG LogonId,
	ULONG TargetLogonId,
	PSTR pPassword,
	BOOL bWait
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateServersW(
	LPWSTR pDomainName,
	DWORD Reserved,
	DWORD Version,
	PWTS_SERVER_INFOW* ppServerInfo,
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateServersA(
	LPSTR pDomainName,
	DWORD Reserved,
	DWORD Version,
	PWTS_SERVER_INFOA* ppServerInfo,
	DWORD* pCount
)
{
	return FALSE;
}

HANDLE WINAPI
FreeRDS_WTSOpenServerW(LPWSTR pServerName)
{
	return NULL;
}

HANDLE WINAPI
FreeRDS_WTSOpenServerA(LPSTR pServerName)
{
	return NULL;
}

HANDLE WINAPI FreeRDS_WTSOpenServerExW(LPWSTR pServerName)
{
	return FreeRDS_WTSOpenServerW(pServerName);
}

HANDLE WINAPI
FreeRDS_WTSOpenServerExA(LPSTR pServerName)
{
	return FreeRDS_WTSOpenServerA(pServerName);
}

VOID WINAPI
FreeRDS_WTSCloseServer(HANDLE hServer)
{
}

BOOL WINAPI
FreeRDS_WTSEnumerateSessionsA(
	HANDLE hServer,
	DWORD Reserved,
	DWORD Version,
	PWTS_SESSION_INFOA* ppSessionInfo,
	DWORD* pCount)
{
	FDSAPI_MESSAGE requestMsg;
	FDSAPI_MESSAGE responseMsg;

	PWTS_SESSION_INFOA pSessionInfoA;
	BOOL bSuccess;
	LPBYTE pExtra;
	DWORD cbExtra;
	DWORD index;
	DWORD count;
	DWORD size;

	*ppSessionInfo = NULL;
	*pCount = 0;

	if (!ConnectClient())
		return FALSE;

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE)
		return FALSE;

	if (Version != 1)
		return FALSE;

	if (!ppSessionInfo)
		return FALSE;

	if (!pCount)
		return FALSE;

	*ppSessionInfo = NULL;
	*pCount = 0;

	/* Execute session manager RPC. */
	ZeroMemory(&requestMsg, sizeof(FDSAPI_MESSAGE));
	ZeroMemory(&responseMsg, sizeof(FDSAPI_MESSAGE));

	requestMsg.messageId = FDSAPI_ENUMERATE_SESSIONS_REQUEST_ID;
	requestMsg.u.enumerateSessionsRequest.version = Version;

	bSuccess = FDSAPI_SendRequest(&requestMsg, &responseMsg);

	if (bSuccess)
	{
		WLog_Print(g_logger, WLOG_DEBUG,
			"WTSEnumerateSessionsA: result=%u, count=%u",
			responseMsg.u.enumerateSessionsResponse.result,
			responseMsg.u.enumerateSessionsResponse.cSessions);

		bSuccess = responseMsg.u.enumerateSessionsResponse.result;
	}

	if (!bSuccess)
		return FALSE;

	count = responseMsg.u.enumerateSessionsResponse.cSessions;

	if (count == 0)
		return TRUE;

	/* Allocate memory (including space for strings). */
	cbExtra = 0;
	for (index = 0; index < count; index++)
	{
		FDSAPI_SESSION_INFO* pSessionInfo = &responseMsg.u.enumerateSessionsResponse.pSessionInfo[index];
		cbExtra += pSessionInfo->winStationName ? strlen(pSessionInfo->winStationName) + 1 : 0;
	}

	size = (count * sizeof(WTS_SESSION_INFOA)) + cbExtra;
	pSessionInfoA = (PWTS_SESSION_INFOA) calloc(1, size);

	if (!pSessionInfoA)
		return FALSE;

	pExtra = (LPBYTE) pSessionInfoA + (count * sizeof(WTS_SESSION_INFOA));

	/* Fill memory with session information. */
	for (index = 0; index < count; index++)
	{
		FDSAPI_SESSION_INFO* pSessionInfo = &responseMsg.u.enumerateSessionsResponse.pSessionInfo[index];

		pSessionInfoA[index].SessionId = (DWORD) pSessionInfo->sessionId;
		pSessionInfoA[index].pWinStationName = (LPSTR) pExtra;
		pSessionInfoA[index].State = (WTS_CONNECTSTATE_CLASS) pSessionInfo->connectState;

		size = pSessionInfo->winStationName ? strlen(pSessionInfo->winStationName) : 0;
		if (size > 0)
		{
			strcpy((LPSTR) pExtra, pSessionInfo->winStationName);
		}
		pExtra += size + 1;
	}

	*ppSessionInfo = pSessionInfoA;
	*pCount = count;

	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateSessionsW(
	HANDLE hServer,
	DWORD Reserved,
	DWORD Version,
	PWTS_SESSION_INFOW* ppSessionInfo,
	DWORD* pCount
)
{
	PWTS_SESSION_INFOA pSessionInfoA;
	PWTS_SESSION_INFOW pSessionInfoW;
	BOOL bSuccess;
	LPBYTE pExtra;
	DWORD cbExtra;
	DWORD count;
	DWORD index;
	DWORD size;

	*ppSessionInfo = NULL;
	*pCount = 0;

	if (!ConnectClient())
		return FALSE;

	bSuccess = FreeRDS_WTSEnumerateSessionsA(
		hServer,
		Reserved,
		Version,
		&pSessionInfoA,
		&count);

	if (bSuccess)
	{
		if (count == 0) return TRUE;

		/* Allocate memory (including space for strings). */
		cbExtra = 0;
		for (index = 0; index < count; index++)
		{
			cbExtra += MultiByteToWideChar(CP_ACP, 0, pSessionInfoA[index].pWinStationName, -1, NULL, 0);
		}

		size = (count * sizeof(WTS_SESSION_INFOW)) + cbExtra;
		pSessionInfoW = (PWTS_SESSION_INFOW)malloc(size);
		if (pSessionInfoW == NULL)
		{
			WTSFreeMemory(pSessionInfoA);
			return FALSE;
		}
		ZeroMemory(pSessionInfoW, size);

		pExtra = (LPBYTE)pSessionInfoW + (count * sizeof(WTS_SESSION_INFOW));

		/* Fill memory with session information. */
		for (index = 0; index < count; index++)
		{
			DWORD cbWinStationName;

			pSessionInfoW[index].SessionId = pSessionInfoA[index].SessionId;
			pSessionInfoW[index].State = pSessionInfoA[index].State;

			if (pSessionInfoA[index].pWinStationName)
			{
				pSessionInfoW[index].pWinStationName = (LPWSTR)pExtra;

				size = MultiByteToWideChar(CP_ACP, 0,
					pSessionInfoA[index].pWinStationName, -1,
					pSessionInfoW[index].pWinStationName, cbExtra);

				pExtra += size;
				cbExtra -= size;
			}
		}
	}

	*ppSessionInfo = pSessionInfoW;
	*pCount = count;	

	return bSuccess;
}

BOOL WINAPI
FreeRDS_WTSEnumerateSessionsExW(
	HANDLE hServer,
	DWORD* pLevel,
	DWORD Filter,
	PWTS_SESSION_INFO_1W* ppSessionInfo,
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateSessionsExA(
	HANDLE hServer,
	DWORD* pLevel,
	DWORD Filter,
	PWTS_SESSION_INFO_1A* ppSessionInfo,
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateProcessesW(
	HANDLE hServer,
	DWORD Reserved,
	DWORD Version,
	PWTS_PROCESS_INFOW* ppProcessInfo,
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateProcessesA(
	HANDLE hServer,
	DWORD Reserved,
	DWORD Version,
	PWTS_PROCESS_INFOA* ppProcessInfo,
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSTerminateProcess(
	HANDLE hServer,
	DWORD ProcessId,
	DWORD ExitCode
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSQuerySessionInformationA(
	HANDLE hServer,
	DWORD SessionId,
	WTS_INFO_CLASS WTSInfoClass,
	LPSTR* ppBuffer,
	DWORD* pBytesReturned
)
{
	BOOL bSuccess;
	FDSAPI_MESSAGE requestMsg;
	FDSAPI_MESSAGE responseMsg;
	FDSAPI_SESSION_INFO_VALUE* infoValue;

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!CheckSessionId(&SessionId))
		return FALSE;

	if (!ppBuffer)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!pBytesReturned)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	/* Connect to the session manager. */
	if (!ConnectClient())
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return FALSE;
	}

	*ppBuffer = NULL;
	*pBytesReturned = 0;

	ZeroMemory(&requestMsg, sizeof(FDSAPI_MESSAGE));
	ZeroMemory(&responseMsg, sizeof(FDSAPI_MESSAGE));

	requestMsg.messageId = FDSAPI_QUERY_SESSION_INFORMATION_REQUEST_ID;
	requestMsg.u.querySessionInformationRequest.sessionId = SessionId;
	requestMsg.u.querySessionInformationRequest.infoClass = WTSInfoClass;

	infoValue = &responseMsg.u.querySessionInformationResponse.infoValue;

	bSuccess = FDSAPI_SendRequest(&requestMsg, &responseMsg);

	if (!bSuccess)
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return FALSE;
	}

	if (!responseMsg.u.querySessionInformationResponse.result)
	{
		FDSAPI_FreeMessage(&responseMsg);
		SetLastError(ERROR_INTERNAL_ERROR);
		return FALSE;
	}

	/* Return the result. */
	switch (WTSInfoClass)
	{
		case WTSSessionId:
		case WTSConnectState:
		case WTSClientBuildNumber:
		case WTSClientHardwareId:
		{
			ULONG* pulValue = (ULONG*) malloc(sizeof(ULONG));

			if (!pulValue)
			{
				WLog_Print(g_logger, WLOG_ERROR, "memory allocation error");
				FDSAPI_FreeMessage(&responseMsg);
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return FALSE;
			}

			*pulValue = (ULONG) infoValue->u.uint32Value;

			*ppBuffer = (LPSTR) pulValue;
			*pBytesReturned = sizeof(ULONG);

			break;
		}

		case WTSClientProductId:
		case WTSClientProtocolType:
		{
			USHORT* pusValue = (USHORT*) malloc(sizeof(USHORT));

			if (!pusValue)
			{
				WLog_Print(g_logger, WLOG_ERROR, "memory allocation error");
				FDSAPI_FreeMessage(&responseMsg);
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return FALSE;
			}

			*pusValue = (USHORT) infoValue->u.uint16Value;

			*ppBuffer = (LPSTR) pusValue;
			*pBytesReturned = sizeof(USHORT);

			break;
		}

		case WTSUserName:
		case WTSWinStationName:
		case WTSDomainName:
		case WTSClientName:
		{
			const char* stringValue = infoValue->u.stringValue ? infoValue->u.stringValue : "";
			int size = strlen(stringValue) + 1;

			LPSTR pszValue = (LPSTR) malloc(size);

			if (!pszValue)
			{
				WLog_Print(g_logger, WLOG_ERROR, "memory allocation error");
				FDSAPI_FreeMessage(&responseMsg);
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return FALSE;
			}

			strncpy(pszValue, stringValue, size);

			*ppBuffer = pszValue;
			*pBytesReturned = size;

			break;
		}

		case WTSClientAddress:
		{
			int ipAddr[4];
			const char* stringValue = infoValue->u.stringValue ? infoValue->u.stringValue : "";

			int size = sizeof(WTS_CLIENT_ADDRESS);
			WTS_CLIENT_ADDRESS* pClientAddress = (WTS_CLIENT_ADDRESS*) calloc(1, size);

			if (!pClientAddress)
			{
				WLog_Print(g_logger, WLOG_ERROR, "memory allocation error");
				FDSAPI_FreeMessage(&responseMsg);
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return FALSE;
			}

			/* TODO: Need to convert IPV4 or IPV6 address from string to binary. */

			pClientAddress->AddressFamily = AF_INET;

			if (sscanf(stringValue, "%d.%d.%d.%d", &ipAddr[0], &ipAddr[1], &ipAddr[2], &ipAddr[3]) == 4)
			{
				pClientAddress->Address[2] = (BYTE)ipAddr[0];
				pClientAddress->Address[3] = (BYTE)ipAddr[1];
				pClientAddress->Address[4] = (BYTE)ipAddr[2];
				pClientAddress->Address[5] = (BYTE)ipAddr[3];
			}

			*ppBuffer = (LPSTR) pClientAddress;
			*pBytesReturned = size;

			break;
		}

		case WTSClientDisplay:
		{
			int size = sizeof(WTS_CLIENT_DISPLAY);
			WTS_CLIENT_DISPLAY* pClientDisplay = (WTS_CLIENT_DISPLAY*) malloc(size);

			if (!pClientDisplay)
			{
				WLog_Print(g_logger, WLOG_ERROR, "memory allocation error");
				FDSAPI_FreeMessage(&responseMsg);
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return FALSE;
			}

			pClientDisplay->HorizontalResolution = infoValue->u.displayValue.displayWidth;
			pClientDisplay->VerticalResolution = infoValue->u.displayValue.displayHeight;
			pClientDisplay->ColorDepth = infoValue->u.displayValue.colorDepth;

			*ppBuffer = (LPSTR) pClientDisplay;
			*pBytesReturned = size;

			break;
		}
	
		default:
			bSuccess = FALSE;
			break;
	}

	FDSAPI_FreeMessage(&responseMsg);

	return bSuccess;
}

BOOL WINAPI
FreeRDS_WTSQuerySessionInformationW(
	HANDLE hServer,
	DWORD SessionId,
	WTS_INFO_CLASS WTSInfoClass,
	LPWSTR* ppBuffer,
	DWORD* pBytesReturned
)
{
	LPSTR pBuffer;
	DWORD cbBuffer;
	BOOL bSuccess;

	*ppBuffer = NULL;
	*pBytesReturned = 0;

	bSuccess = FreeRDS_WTSQuerySessionInformationA(
		hServer,
		SessionId,
		WTSInfoClass,
		&pBuffer,
		&cbBuffer);

	if (bSuccess)
	{
		switch (WTSInfoClass)
		{
			case WTSUserName:
			case WTSWinStationName:
			case WTSDomainName:
			case WTSClientName:
			{
				int status = ConvertToUnicode(CP_ACP, 0, pBuffer, -1, ppBuffer, 0);
				if (status > 0)
				{
					*pBytesReturned = status;
				}
				else
				{
					bSuccess = FALSE;
				}
				WTSFreeMemory(pBuffer);
				break;
			}

			default:
				*ppBuffer = (LPWSTR)pBuffer;
				*pBytesReturned = cbBuffer;
				break;
		}
	}

	return bSuccess;
}

BOOL WINAPI
FreeRDS_WTSQueryUserConfigW(
	LPWSTR pServerName,
	LPWSTR pUserName,
	WTS_CONFIG_CLASS WTSConfigClass,
	LPWSTR* ppBuffer,
	DWORD* pBytesReturned
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSQueryUserConfigA(
	LPSTR pServerName,
	LPSTR pUserName,
	WTS_CONFIG_CLASS WTSConfigClass,
	LPSTR* ppBuffer,
	DWORD* pBytesReturned
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSSetUserConfigW(
	LPWSTR pServerName,
	LPWSTR pUserName,
	WTS_CONFIG_CLASS WTSConfigClass,
	LPWSTR pBuffer,
	DWORD DataLength
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSSetUserConfigA(
	LPSTR pServerName,
	LPSTR pUserName,
	WTS_CONFIG_CLASS WTSConfigClass,
	LPSTR pBuffer,
	DWORD DataLength
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSSendMessageW(
	HANDLE hServer,
	DWORD SessionId,
	LPWSTR pTitle,
	DWORD TitleLength,
	LPWSTR pMessage,
	DWORD MessageLength,
	DWORD Style,
	DWORD Timeout,
	DWORD* pResponse,
	BOOL bWait
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSSendMessageA(
	HANDLE hServer,
	DWORD SessionId,
	LPSTR pTitle,
	DWORD TitleLength,
	LPSTR pMessage,
	DWORD MessageLength,
	DWORD Style,
	DWORD Timeout,
	DWORD* pResponse,
	BOOL bWait
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSDisconnectSession(
	HANDLE hServer,
	DWORD SessionId,
	BOOL bWait
)
{
	BOOL bSuccess;
	FDSAPI_MESSAGE requestMsg;
	FDSAPI_MESSAGE responseMsg;

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!CheckSessionId(&SessionId))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	/* Connect to the session manager. */
	if (!ConnectClient())
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return FALSE;
	}

	/* Execute session manager RPC. */
	ZeroMemory(&requestMsg, sizeof(FDSAPI_MESSAGE));
	ZeroMemory(&responseMsg, sizeof(FDSAPI_MESSAGE));

	requestMsg.messageId = FDSAPI_DISCONNECT_SESSION_REQUEST_ID;
	requestMsg.u.disconnectSessionRequest.sessionId = SessionId;
	requestMsg.u.disconnectSessionRequest.wait = bWait;

	bSuccess = FDSAPI_SendRequest(&requestMsg, &responseMsg);

	if (bSuccess)
	{
		bSuccess = responseMsg.u.disconnectSessionResponse.result;

		FDSAPI_FreeMessage(&responseMsg);
	}

	/* Return the result. */
	return bSuccess;
}

BOOL WINAPI
FreeRDS_WTSLogoffSession(
	HANDLE hServer,
	DWORD SessionId,
	BOOL bWait
)
{
	BOOL bSuccess;
	FDSAPI_MESSAGE requestMsg;
	FDSAPI_MESSAGE responseMsg;

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!CheckSessionId(&SessionId))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	/* Connect to the session manager. */
	if (!ConnectClient())
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return FALSE;
	}

	/* Execute session manager RPC. */
	ZeroMemory(&requestMsg, sizeof(FDSAPI_MESSAGE));
	ZeroMemory(&responseMsg, sizeof(FDSAPI_MESSAGE));

	requestMsg.messageId = FDSAPI_LOGOFF_SESSION_REQUEST_ID;
	requestMsg.u.logoffSessionRequest.sessionId = SessionId;
	requestMsg.u.logoffSessionRequest.wait = bWait;

	bSuccess = FDSAPI_SendRequest(&requestMsg, &responseMsg);

	if (bSuccess)
	{
		bSuccess = responseMsg.u.logoffSessionResponse.result;

		FDSAPI_FreeMessage(&responseMsg);
	}

	/* Return the result. */
	return bSuccess;
}

BOOL WINAPI
FreeRDS_WTSShutdownSystem(
	HANDLE hServer,
	DWORD ShutdownFlag
)
{
	FDSAPI_MESSAGE requestMsg;
	FDSAPI_MESSAGE responseMsg;
	BOOL bSuccess;

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	/* Connect to the session manager. */
	if (!ConnectClient())
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return FALSE;
	}

	/* Execute session manager RPC. */
	ZeroMemory(&requestMsg, sizeof(FDSAPI_MESSAGE));
	ZeroMemory(&responseMsg, sizeof(FDSAPI_MESSAGE));

	requestMsg.messageId = FDSAPI_SHUTDOWN_SYSTEM_REQUEST_ID;
	requestMsg.u.shutdownSystemRequest.shutdownFlag = ShutdownFlag;

	bSuccess = FDSAPI_SendRequest(&requestMsg, &responseMsg);

	if (bSuccess)
	{
		bSuccess = responseMsg.u.shutdownSystemResponse.result;

		FDSAPI_FreeMessage(&responseMsg);
	}

	/* Return the result. */
	return bSuccess;
}

BOOL WINAPI
FreeRDS_WTSWaitSystemEvent(
	HANDLE hServer,
	DWORD EventMask,
	DWORD* pEventFlags
)
{
	DWORD dwErrorCode;
	BOOL bSuccess = FALSE;
	HANDLE hEvent = NULL;
	FDSAPI_SESSION_EVENT_OBSERVER* pSessionEventObserver = NULL;

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	/* Connect to the FDSAPI server. */
	if (!ConnectClient())
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return FALSE;
	}

	/* Create an event. */
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!hEvent)
	{
		dwErrorCode = GetLastError();
		WLog_Print(g_logger, WLOG_ERROR, "error creating event");
		goto CLEANUP;
	}

	/* Create a session event observer. */
	pSessionEventObserver = (FDSAPI_SESSION_EVENT_OBSERVER*) calloc(1, sizeof(FDSAPI_SESSION_EVENT_OBSERVER));

	if (!pSessionEventObserver)
	{
		dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
		WLog_Print(g_logger, WLOG_ERROR, "memory allocation error");
		goto CLEANUP;
	}

	pSessionEventObserver->type = FDSAPI_WAIT_SYSTEM_EVENT_TYPE;
	pSessionEventObserver->u.waitSystemEvent.hWaitEvent = hEvent;
	pSessionEventObserver->u.waitSystemEvent.dwEventMask = EventMask;

	/* Add the session event observer to a list. */
	ArrayList_Lock(g_sessionEventObserverList);
	ArrayList_Add(g_sessionEventObserverList, pSessionEventObserver);
	ArrayList_Unlock(g_sessionEventObserverList);

	/* Wait for the event. */
	if (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)
	{
		bSuccess = TRUE;
		dwErrorCode = ERROR_SUCCESS;
	}
	else
	{
		dwErrorCode = GetLastError();
		WLog_Print(g_logger, WLOG_ERROR, "event wait failed");
	}

	/* Remove the session event observer from a list. */
	ArrayList_Lock(g_sessionEventObserverList);
	ArrayList_Remove(g_sessionEventObserverList, pSessionEventObserver);
	ArrayList_Unlock(g_sessionEventObserverList);

	if (bSuccess)
	{
		/* Return the event flags to the caller. */
		*pEventFlags = pSessionEventObserver->u.waitSystemEvent.dwEventFlags;
	}

CLEANUP:
	if (hEvent)
	{
		CloseHandle(hEvent);
	}

	if (pSessionEventObserver)
	{
		free(pSessionEventObserver);
	}

	SetLastError(dwErrorCode);

	return bSuccess;
}

HANDLE WINAPI
FreeRDS_WTSVirtualChannelOpen(
	HANDLE hServer,
	DWORD SessionId,
	LPSTR pVirtualName
)
{
	int status;
	BOOL bSuccess;
	DWORD dwErrorCode;
	UINT32 channelPort;
	const char* channelGuid;
	FDSAPI_MESSAGE requestMsg;
	FDSAPI_MESSAGE responseMsg;
	FDSAPI_CHANNEL* pChannel = NULL;

	if (!pVirtualName)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!ConnectClient())
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return NULL;
	}

	if (hServer != WTS_CURRENT_SERVER_HANDLE)
	{
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	if (!CheckSessionId(&SessionId))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	WLog_Print(g_logger, WLOG_DEBUG, "WTSVirtualChannelOpen: %s", pVirtualName);

	/* Execute session manager RPC. */
	ZeroMemory(&requestMsg, sizeof(FDSAPI_MESSAGE));
	ZeroMemory(&responseMsg, sizeof(FDSAPI_MESSAGE));

	requestMsg.messageId = FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST_ID;
	requestMsg.u.virtualChannelOpenRequest.sessionId = SessionId;
	requestMsg.u.virtualChannelOpenRequest.virtualName = pVirtualName;

	bSuccess = FDSAPI_SendRequest(&requestMsg, &responseMsg);

	if (!bSuccess)
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return NULL;
	}

	channelPort = responseMsg.u.virtualChannelOpenResponse.channelPort;
	channelGuid = responseMsg.u.virtualChannelOpenResponse.channelGuid;

	WLog_Print(g_logger, WLOG_DEBUG, "WTSVirtualChannelOpen: %s:%d", channelGuid, channelPort);

	if (!channelGuid || !channelPort)
	{
		dwErrorCode = ERROR_INTERNAL_ERROR;
		goto CLEANUP;
	}

	pChannel = FDSAPI_Channel_New();

	if (!pChannel)
	{
		dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
		goto CLEANUP;
	}

	status = FDSAPI_Channel_Connect(pChannel, channelGuid, channelPort);

	if (status < 0)
	{
		dwErrorCode = ERROR_OPEN_FAILED;
		WLog_Print(g_logger, WLOG_ERROR, "FDSAPI_Channel_Connect failure: %d", status);
		goto CLEANUP;
	}

	FDSAPI_FreeMessage(&responseMsg);

	SetLastError(ERROR_SUCCESS);

	return (HANDLE) pChannel;

CLEANUP:
	if (pChannel)
	{
		FDSAPI_Channel_Free(pChannel);
	}

	FDSAPI_FreeMessage(&responseMsg);

	SetLastError(dwErrorCode);

	return NULL;
}

HANDLE WINAPI
FreeRDS_WTSVirtualChannelOpenEx(
	DWORD SessionId,
	LPSTR pVirtualName,
	DWORD flags
)
{
	int status;
	BOOL bSuccess;
	DWORD dwErrorCode;
	UINT32 channelPort;
	const char* channelGuid;
	FDSAPI_MESSAGE requestMsg;
	FDSAPI_MESSAGE responseMsg;
	FDSAPI_CHANNEL* pChannel = NULL;

	if (!pVirtualName)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!ConnectClient())
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return NULL;
	}

	if (!CheckSessionId(&SessionId))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	WLog_Print(g_logger, WLOG_DEBUG, "WTSVirtualChannelOpenEx: %s, 0x%x", pVirtualName, flags);

	/* Execute session manager RPC. */
	ZeroMemory(&requestMsg, sizeof(FDSAPI_MESSAGE));
	ZeroMemory(&responseMsg, sizeof(FDSAPI_MESSAGE));

	requestMsg.messageId = FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST_ID;
	requestMsg.u.virtualChannelOpenExRequest.sessionId = SessionId;
	requestMsg.u.virtualChannelOpenExRequest.virtualName = pVirtualName;

	bSuccess = FDSAPI_SendRequest(&requestMsg, &responseMsg);

	if (!bSuccess)
	{
		SetLastError(ERROR_INTERNAL_ERROR);
		return NULL;
	}

	channelPort = responseMsg.u.virtualChannelOpenExResponse.channelPort;
	channelGuid = responseMsg.u.virtualChannelOpenExResponse.channelGuid;

	WLog_Print(g_logger, WLOG_DEBUG, "WTSVirtualChannelOpenEx: %s:%d", channelGuid, channelPort);

	if (!channelGuid || !channelPort)
	{
		dwErrorCode = ERROR_INTERNAL_ERROR;
		goto CLEANUP;
	}

	pChannel = FDSAPI_Channel_New();

	if (!pChannel)
	{
		dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
		goto CLEANUP;
	}

	status = FDSAPI_Channel_Connect(pChannel, channelGuid, channelPort);

	if (status < 0)
	{
		dwErrorCode = ERROR_OPEN_FAILED;
		WLog_Print(g_logger, WLOG_ERROR, "FDSAPI_Channel_Connect failure: %d", status);
		goto CLEANUP;
	}

	FDSAPI_FreeMessage(&responseMsg);

	SetLastError(ERROR_SUCCESS);

	return (HANDLE) pChannel;

CLEANUP:
	if (pChannel)
	{
		FDSAPI_Channel_Free(pChannel);
	}

	FDSAPI_FreeMessage(&responseMsg);

	SetLastError(dwErrorCode);

	return NULL;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelClose(
	HANDLE hChannelHandle
)
{
	FDSAPI_CHANNEL* pChannel;

	WLog_Print(g_logger, WLOG_DEBUG, "WTSVirtualChannelClose: %p", hChannelHandle);

	if (!hChannelHandle)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	pChannel = (FDSAPI_CHANNEL*) hChannelHandle;

	FDSAPI_Channel_Free(pChannel);

	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelRead(
	HANDLE hChannelHandle,
	ULONG TimeOut,
	PCHAR Buffer,
	ULONG BufferSize,
	PULONG pBytesRead
)
{
	int status;
	DWORD waitStatus;
	FDSAPI_CHANNEL* pChannel;

	WLog_Print(g_logger, WLOG_DEBUG, "WTSVirtualChannelRead: %p %lu %p %lu %p", hChannelHandle, TimeOut, Buffer, BufferSize, pBytesRead);

	if (!hChannelHandle)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (!Buffer && BufferSize)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!pBytesRead)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	pChannel = (FDSAPI_CHANNEL*) hChannelHandle;

	*pBytesRead = 0;

	if (TimeOut)
	{
		waitStatus = WaitForSingleObject(pChannel->event, TimeOut);

		if (waitStatus == WAIT_TIMEOUT)
		{
			SetLastError(ERROR_IO_INCOMPLETE);
			return FALSE;
		}
		else if (waitStatus != WAIT_OBJECT_0)
		{
			SetLastError(ERROR_INTERNAL_ERROR);
			return FALSE;
		}
	}

	status = _recv(pChannel->socket, Buffer, BufferSize, 0);

	if (status <= 0)
	{
		SetLastError(ERROR_BROKEN_PIPE);
		return FALSE;
	}

	*pBytesRead = (ULONG) status;

	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelWrite(
	HANDLE hChannelHandle,
	PCHAR Buffer,
	ULONG Length,
	PULONG pBytesWritten
)
{
	int status;
	FDSAPI_CHANNEL* pChannel;

	WLog_Print(g_logger, WLOG_DEBUG, "WTSVirtualChannelWrite: %p %p %lu %p", hChannelHandle, Buffer, Length, pBytesWritten);

	if (!hChannelHandle)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (!Buffer && Length)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!pBytesWritten)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	pChannel = (FDSAPI_CHANNEL*) hChannelHandle;

	*pBytesWritten = 0;

	status = _send(pChannel->socket, Buffer, Length, 0);

	if (status < 0)
	{
		SetLastError(ERROR_BROKEN_PIPE);
		return FALSE;
	}

	*pBytesWritten = (ULONG) status;

	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelPurgeInput(
	HANDLE hChannelHandle
)
{
	if (!hChannelHandle)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelPurgeOutput(
	HANDLE hChannelHandle
)
{
	if (!hChannelHandle)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelQuery(
	HANDLE hChannelHandle,
	WTS_VIRTUAL_CLASS WtsVirtualClass,
	PVOID* ppBuffer,
	DWORD* pBytesReturned
)
{
	FDSAPI_CHANNEL* pChannel;

	if (!hChannelHandle)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (!ppBuffer || !pBytesReturned)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	pChannel = (FDSAPI_CHANNEL*) hChannelHandle;

	if (WtsVirtualClass == WTSVirtualFileHandle)
	{
		/* overlapped i/o is not supported */
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	else if (WtsVirtualClass == WTSVirtualEventHandle)
	{
		*pBytesReturned = sizeof(HANDLE);
		*ppBuffer = malloc(*pBytesReturned);

		if (*ppBuffer == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return FALSE;
		}

		CopyMemory(*ppBuffer, &(pChannel->event), *pBytesReturned);
	}
	else
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	return TRUE;
}

VOID WINAPI
FreeRDS_WTSFreeMemory(
	PVOID pMemory
)
{
	if (pMemory)
	{
		free(pMemory);
	}
}

BOOL WINAPI
FreeRDS_WTSFreeMemoryExW(
	WTS_TYPE_CLASS WTSTypeClass,
	PVOID pMemory,
	ULONG NumberOfEntries
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSFreeMemoryExA(
	WTS_TYPE_CLASS WTSTypeClass,
	PVOID pMemory,
	ULONG NumberOfEntries
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSRegisterSessionNotification(
	HWND hWnd, 
	DWORD dwFlags
)
{
	FDSAPI_SESSION_EVENT_OBSERVER* pSessionEventObserver = NULL;

	if (!ConnectClient())
		return FALSE;

	/* Create a session event observer. */
	pSessionEventObserver = (FDSAPI_SESSION_EVENT_OBSERVER*) calloc(1, sizeof(FDSAPI_SESSION_EVENT_OBSERVER));

	if (!pSessionEventObserver)
		return FALSE;

	pSessionEventObserver->type = FDSAPI_SESSION_NOTIFICATION_TYPE;
	pSessionEventObserver->u.sessionNotification.hWnd = hWnd;
	pSessionEventObserver->u.sessionNotification.dwFlags = dwFlags;

	/* Add the session event observer to a list. */
	ArrayList_Lock(g_sessionEventObserverList);
	ArrayList_Add(g_sessionEventObserverList, pSessionEventObserver);
	ArrayList_Unlock(g_sessionEventObserverList);

	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSUnRegisterSessionNotification(
	HWND hWnd
)
{
	int index;
	BOOL bSuccess = FALSE;
	FDSAPI_SESSION_EVENT_OBSERVER* pSessionEventObserver;

	if (!ConnectClient())
		return FALSE;

	/* Loop through the session event observer list... */
	ArrayList_Lock(g_sessionEventObserverList);

	for (index = 0; !bSuccess && (index < ArrayList_Count(g_sessionEventObserverList)); index++)
	{
		/* Get the next item in the list. */
		pSessionEventObserver = (FDSAPI_SESSION_EVENT_OBSERVER*) ArrayList_GetItem(g_sessionEventObserverList, index);

		if (pSessionEventObserver->type == FDSAPI_SESSION_NOTIFICATION_TYPE)
		{
			/* If the item matches the specified HWND... */
			if (pSessionEventObserver->u.sessionNotification.hWnd == hWnd)
			{
				/* Remove the item from the list. */
				ArrayList_RemoveAt(g_sessionEventObserverList, index);

				/* Free the session event observer. */
				free(pSessionEventObserver);

				bSuccess = TRUE;
			}
		}
	}

	ArrayList_Unlock(g_sessionEventObserverList);

	return bSuccess;
}

BOOL WINAPI
FreeRDS_WTSRegisterSessionNotificationEx(
	HANDLE hServer,
	HWND hWnd,
	DWORD dwFlags
)
{	
	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE)
		return FALSE;

	return FreeRDS_WTSRegisterSessionNotification(hWnd, dwFlags);
}

BOOL WINAPI
FreeRDS_WTSUnRegisterSessionNotificationEx(
	HANDLE hServer,
	HWND hWnd
)
{
	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE)
		return FALSE;

	return FreeRDS_WTSUnRegisterSessionNotification(hWnd);
}

BOOL WINAPI
FreeRDS_WTSQueryUserToken(
	ULONG SessionId,
	PHANDLE phToken
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateProcessesExW(
	HANDLE hServer,
	DWORD* pLevel,
	DWORD SessionId,
	LPWSTR* ppProcessInfo,
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateProcessesExA(
	HANDLE hServer,
	DWORD* pLevel,
	DWORD SessionId,
	LPSTR* ppProcessInfo,
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateListenersW(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	PWTSLISTENERNAMEW pListeners, 
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSEnumerateListenersA(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	PWTSLISTENERNAMEA pListeners,
	DWORD* pCount
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSQueryListenerConfigW(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	LPWSTR pListenerName,
	PWTSLISTENERCONFIGW pBuffer
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSQueryListenerConfigA(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	LPSTR pListenerName,
	PWTSLISTENERCONFIGA pBuffer
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSCreateListenerW(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	LPWSTR pListenerName,
	PWTSLISTENERCONFIGW pBuffer,
	DWORD flag
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSCreateListenerA(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	LPSTR pListenerName,
	PWTSLISTENERCONFIGA pBuffer,
	DWORD flag
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSSetListenerSecurityW(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	LPWSTR pListenerName,
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSSetListenerSecurityA(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	LPSTR pListenerName,
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor
)
{
	return FALSE;
}

BOOL WINAPI 
FreeRDS_WTSGetListenerSecurityW(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	LPWSTR pListenerName,
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	DWORD nLength,
	LPDWORD lpnLengthNeeded
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSGetListenerSecurityA(
	HANDLE hServer,
	PVOID pReserved,
	DWORD Reserved,
	LPSTR pListenerName,
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	DWORD nLength,
	LPDWORD lpnLengthNeeded
)
{
	return FALSE;
}

BOOL CDECL
FreeRDS_WTSEnableChildSessions(
	BOOL bEnable
)
{
	return FALSE;
}

BOOL CDECL
FreeRDS_WTSIsChildSessionsEnabled(
	PBOOL pbEnabled
)
{
	return FALSE;
}

BOOL CDECL
FreeRDS_WTSGetChildSessionId(
	PULONG pSessionId
)
{
	return FALSE;
}

DWORD WINAPI
FreeRDS_WTSGetActiveConsoleSessionId(void)
{
	return 0xFFFFFFFF;
}

static WtsApiFunctionTable FreeRDS_WtsApiFunctionTable =
{
	0, /* dwVersion */
	0, /* dwFlags */

	FreeRDS_WTSStopRemoteControlSession, /* StopRemoteControlSession */
	FreeRDS_WTSStartRemoteControlSessionW, /* StartRemoteControlSessionW */
	FreeRDS_WTSStartRemoteControlSessionA, /* StartRemoteControlSessionA */
	FreeRDS_WTSConnectSessionW, /* ConnectSessionW */
	FreeRDS_WTSConnectSessionA, /* ConnectSessionA */
	FreeRDS_WTSEnumerateServersW, /* EnumerateServersW */
	FreeRDS_WTSEnumerateServersA, /* EnumerateServersA */
	FreeRDS_WTSOpenServerW, /* OpenServerW */
	FreeRDS_WTSOpenServerA, /* OpenServerA */
	FreeRDS_WTSOpenServerExW, /* OpenServerExW */
	FreeRDS_WTSOpenServerExA, /* OpenServerExA */
	FreeRDS_WTSCloseServer, /* CloseServer */
	FreeRDS_WTSEnumerateSessionsW, /* EnumerateSessionsW */
	FreeRDS_WTSEnumerateSessionsA, /* EnumerateSessionsA */
	FreeRDS_WTSEnumerateSessionsExW, /* EnumerateSessionsExW */
	FreeRDS_WTSEnumerateSessionsExA, /* EnumerateSessionsExA */
	FreeRDS_WTSEnumerateProcessesW, /* EnumerateProcessesW */
	FreeRDS_WTSEnumerateProcessesA, /* EnumerateProcessesA */
	FreeRDS_WTSTerminateProcess, /* TerminateProcess */
	FreeRDS_WTSQuerySessionInformationW, /* QuerySessionInformationW */
	FreeRDS_WTSQuerySessionInformationA, /* QuerySessionInformationA */
	FreeRDS_WTSQueryUserConfigW, /* QueryUserConfigW */
	FreeRDS_WTSQueryUserConfigA, /* QueryUserConfigA */
	FreeRDS_WTSSetUserConfigW, /* SetUserConfigW */
	FreeRDS_WTSSetUserConfigA, /* SetUserConfigA */
	FreeRDS_WTSSendMessageW, /* SendMessageW */
	FreeRDS_WTSSendMessageA, /* SendMessageA */
	FreeRDS_WTSDisconnectSession, /* DisconnectSession */
	FreeRDS_WTSLogoffSession, /* LogoffSession */
	FreeRDS_WTSShutdownSystem, /* ShutdownSystem */
	FreeRDS_WTSWaitSystemEvent, /* WaitSystemEvent */
	FreeRDS_WTSVirtualChannelOpen, /* VirtualChannelOpen */
	FreeRDS_WTSVirtualChannelOpenEx, /* VirtualChannelOpenEx */
	FreeRDS_WTSVirtualChannelClose, /* VirtualChannelClose */
	FreeRDS_WTSVirtualChannelRead, /* VirtualChannelRead */
	FreeRDS_WTSVirtualChannelWrite, /* VirtualChannelWrite */
	FreeRDS_WTSVirtualChannelPurgeInput, /* VirtualChannelPurgeInput */
	FreeRDS_WTSVirtualChannelPurgeOutput, /* VirtualChannelPurgeOutput */
	FreeRDS_WTSVirtualChannelQuery, /* VirtualChannelQuery */
	FreeRDS_WTSFreeMemory, /* FreeMemory */
	FreeRDS_WTSRegisterSessionNotification, /* RegisterSessionNotification */
	FreeRDS_WTSUnRegisterSessionNotification, /* UnRegisterSessionNotification */
	FreeRDS_WTSRegisterSessionNotificationEx, /* RegisterSessionNotificationEx */
	FreeRDS_WTSUnRegisterSessionNotificationEx, /* UnRegisterSessionNotificationEx */
	FreeRDS_WTSQueryUserToken, /* QueryUserToken */
	FreeRDS_WTSFreeMemoryExW, /* FreeMemoryExW */
	FreeRDS_WTSFreeMemoryExA, /* FreeMemoryExA */
	FreeRDS_WTSEnumerateProcessesExW, /* EnumerateProcessesExW */
	FreeRDS_WTSEnumerateProcessesExA, /* EnumerateProcessesExA */
	FreeRDS_WTSEnumerateListenersW, /* EnumerateListenersW */
	FreeRDS_WTSEnumerateListenersA, /* EnumerateListenersA */
	FreeRDS_WTSQueryListenerConfigW, /* QueryListenerConfigW */
	FreeRDS_WTSQueryListenerConfigA, /* QueryListenerConfigA */
	FreeRDS_WTSCreateListenerW, /* CreateListenerW */
	FreeRDS_WTSCreateListenerA, /* CreateListenerA */
	FreeRDS_WTSSetListenerSecurityW, /* SetListenerSecurityW */
	FreeRDS_WTSSetListenerSecurityA, /* SetListenerSecurityA */
	FreeRDS_WTSGetListenerSecurityW, /* GetListenerSecurityW */
	FreeRDS_WTSGetListenerSecurityA, /* GetListenerSecurityA */
	FreeRDS_WTSEnableChildSessions, /* EnableChildSessions */
	FreeRDS_WTSIsChildSessionsEnabled, /* IsChildSessionsEnabled */
	FreeRDS_WTSGetChildSessionId, /* GetChildSessionId */
	FreeRDS_WTSGetActiveConsoleSessionId /* GetActiveConsoleSessionId */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Export InitWtsApi() but do not expose it with a header file due to possible conflicts
 */

WINPR_API PWtsApiFunctionTable CDECL InitWtsApi(void)
{
	return &FreeRDS_WtsApiFunctionTable;
}

/**
 * Export explicit FreeRDS_InitWtsApi() which we can safely expose with a header
 */

WINPR_API PWtsApiFunctionTable CDECL FreeRDS_InitWtsApi(void)
{
	return &FreeRDS_WtsApiFunctionTable;
}

#ifdef __cplusplus
}
#endif



/**
 * WTSAPI Private Interface
 */

int WINAPI
FreeRDS_AuthenticateUser(
	UINT32 SessionId,
	LPCSTR Username,
	LPCSTR Password,
	LPCSTR Domain
)
{
	FDSAPI_MESSAGE requestMsg;
	FDSAPI_MESSAGE responseMsg;
	BOOL bSuccess;
	int authStatus;

	if (!ConnectClient())
		return FALSE;

	/* Check parameters. */
	if (!CheckSessionId((LPDWORD) &SessionId))
		return -1;

	/* Execute session manager RPC. */
	ZeroMemory(&requestMsg, sizeof(FDSAPI_MESSAGE));
	ZeroMemory(&responseMsg, sizeof(FDSAPI_MESSAGE));

	requestMsg.messageId = FDSAPI_AUTHENTICATE_USER_REQUEST_ID;
	requestMsg.u.authenticateUserRequest.sessionId = SessionId;
	requestMsg.u.authenticateUserRequest.username = Username ? _strdup(Username) : NULL;
	requestMsg.u.authenticateUserRequest.password = Password ? _strdup(Password) : NULL;
	requestMsg.u.authenticateUserRequest.domain = Domain ? _strdup(Domain) : NULL;

	bSuccess = FDSAPI_SendRequest(&requestMsg, &responseMsg);

	if (bSuccess)
	{
		authStatus = responseMsg.u.authenticateUserResponse.result;
	}
	else
	{
		authStatus = -1;
	}

	return authStatus;
}
