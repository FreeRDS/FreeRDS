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

#include "fdsapi_thrift.h"

#include <fdsapi/fdsapi.h>

#include <winpr/crt.h>
#include <winpr/pipe.h>
#include <winpr/wtsapi.h>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

boost::shared_ptr<TTransport> gTransport;
boost::shared_ptr<freerds::fdsapiClient> gClient;

std::string gAuthToken("HUGO");

#define CHECK_CLIENT_CONNECTION() if((!gTransport)||(!gTransport->isOpen())) {connectClient();}

static int connectClient()
{
	boost::shared_ptr<TSocket> socket(new TSocket("localhost", 9091));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

	boost::shared_ptr<freerds::fdsapiClient> client(new freerds::fdsapiClient(protocol));

	gClient = client;
	gTransport = transport;
	transport->open();

	return 0;
}


/**
 * Local Utility Functions
 */

static BOOL GetCurrentSessionId(LPDWORD pSessionId)
{
	/**
	 * TODO: Need to figure out how to get the session id
	 * associated with the current process.  Perhaps an
	 * environment variable such as FREERDS_SID can be
	 * used.
	 */
	*pSessionId = 0;
	return TRUE;
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
	freerds::TReturnEnumerateSessions result;
	PWTS_SESSION_INFOA pSessionInfoA;
	BOOL bSuccess;
	LPBYTE pExtra;
	DWORD cbExtra;
	DWORD index;
	DWORD count;
	DWORD size;

	CHECK_CLIENT_CONNECTION();

	*ppSessionInfo = NULL;
	*pCount = 0;

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE) return FALSE;
	if (Version != 1) return FALSE;
	if (ppSessionInfo == NULL) return FALSE;
	if (pCount == NULL) return FALSE;

	*ppSessionInfo = NULL;
	*pCount = 0;

	/* Execute session manager RPC. */
	try
	{
		gClient->enumerateSessions(result, gAuthToken, Version);
		bSuccess = result.returnValue ? TRUE : FALSE;
	}
	catch (...)
	{
		bSuccess = FALSE;
	}
	if (!bSuccess) return FALSE;

	count = (DWORD)result.sessionInfoList.size();
	if (count == 0) return TRUE;

	/* Allocate memory (including space for strings). */
	cbExtra = 0;
	for (index = 0; index < count; index++)
	{
		freerds::TSessionInfo sessionInfo = result.sessionInfoList.at(index);

		cbExtra += sessionInfo.winStationName.length() + 1;
	}

	size = (count * sizeof(WTS_SESSION_INFOA)) + cbExtra;
	pSessionInfoA = (PWTS_SESSION_INFOA)malloc(size);
	if (pSessionInfoA == NULL) return FALSE;
	ZeroMemory(pSessionInfoA, size);

	pExtra = (LPBYTE)pSessionInfoA + (count * sizeof(WTS_SESSION_INFOA));

	/* Fill memory with session information. */
	for (index = 0; index < count; index++)
	{
		freerds::TSessionInfo sessionInfo = result.sessionInfoList.at(index);

		pSessionInfoA[index].SessionId = (DWORD)sessionInfo.sessionId;
		pSessionInfoA[index].pWinStationName = (LPSTR)pExtra;
		pSessionInfoA[index].State = (WTS_CONNECTSTATE_CLASS)sessionInfo.connectState;

		strcpy((LPSTR)pExtra, sessionInfo.winStationName.c_str());
		pExtra += sessionInfo.winStationName.length() + 1;
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

	CHECK_CLIENT_CONNECTION();

	*ppSessionInfo = NULL;
	*pCount = 0;

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
	freerds::TReturnQuerySessionInformation result;
	BOOL bSuccess;

	CHECK_CLIENT_CONNECTION();

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE) return FALSE;
	if (!CheckSessionId(&SessionId)) return FALSE;
	if (ppBuffer == NULL) return FALSE;
	if (pBytesReturned == NULL) return FALSE;

	/* Execute session manager RPC. */
	try
	{
		gClient->querySessionInformation(result, gAuthToken, SessionId, WTSInfoClass);
		bSuccess = result.returnValue ? TRUE : FALSE;
	}
	catch (...)
	{
		bSuccess = FALSE;
	}
	if (!bSuccess) return FALSE;

	/* Return the result. */
	switch (WTSInfoClass)
	{
		case WTSSessionId:
		case WTSConnectState:
		case WTSClientBuildNumber:
		case WTSClientHardwareId:
		{
			ULONG* pulValue = (ULONG*)malloc(sizeof(ULONG));
			if (pulValue == NULL) return FALSE;

			*pulValue = (ULONG)result.infoValue.int32Value;

			*ppBuffer = (LPSTR)pulValue;
			*pBytesReturned = sizeof(ULONG);

			break;
		}

		case WTSClientProductId:
		case WTSClientProtocolType:
		{
			USHORT* pusValue = (USHORT*)malloc(sizeof(USHORT));
			if (pusValue == NULL) return FALSE;

			*pusValue = (USHORT)result.infoValue.int16Value;

			*ppBuffer = (LPSTR)pusValue;
			*pBytesReturned = sizeof(USHORT);

			break;
		}

		case WTSUserName:
		case WTSWinStationName:
		case WTSDomainName:
		case WTSClientName:
		{
			int size = result.infoValue.stringValue.length() + 1;
			LPSTR pszValue = (LPSTR)malloc(size);
			if (pszValue == NULL) return FALSE;

			strncpy(pszValue, result.infoValue.stringValue.c_str(), size);

			*ppBuffer = pszValue;
			*pBytesReturned = size;

			break;
		}

		case WTSClientAddress:
		{
			int size = sizeof(WTS_CLIENT_ADDRESS);
			WTS_CLIENT_ADDRESS* pClientAddress = (WTS_CLIENT_ADDRESS*)malloc(size);
			if (pClientAddress == NULL) return FALSE;

			/* TODO: Need to convert IPV4 or IPV6 address from string to binary. */
			ZeroMemory(pClientAddress, sizeof(WTS_CLIENT_ADDRESS));
			pClientAddress->AddressFamily = AF_INET;

			*ppBuffer = (LPSTR)pClientAddress;
			*pBytesReturned = size;

			break;
		}

		case WTSClientDisplay:
		{
			int size = sizeof(WTS_CLIENT_DISPLAY);
			WTS_CLIENT_DISPLAY* pClientDisplay = (WTS_CLIENT_DISPLAY*)malloc(size);
			if (pClientDisplay == NULL) return FALSE;

			pClientDisplay->HorizontalResolution = result.infoValue.displayValue.displayWidth;
			pClientDisplay->VerticalResolution = result.infoValue.displayValue.displayHeight;
			pClientDisplay->ColorDepth = result.infoValue.displayValue.colorDepth;

			*ppBuffer = (LPSTR)pClientDisplay;
			*pBytesReturned = size;

			break;
		}
	
		default:
			bSuccess = FALSE;
			break;
	}

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

	CHECK_CLIENT_CONNECTION();

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE) return FALSE;
	if (!CheckSessionId(&SessionId)) return FALSE;

	/* Execute session manager RPC. */
	try
	{
		bSuccess = gClient->disconnectSession(gAuthToken, SessionId, bWait);
	}
	catch (...)
	{
		bSuccess = FALSE;
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

	CHECK_CLIENT_CONNECTION();

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE) return FALSE;
	if (!CheckSessionId(&SessionId)) return FALSE;

	/* Execute session manager RPC. */
	try
	{
		bSuccess = gClient->logoffSession(gAuthToken, SessionId, bWait);
	}
	catch (...)
	{
		bSuccess = FALSE;
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
	BOOL bSuccess;

	CHECK_CLIENT_CONNECTION();

	/* Check parameters. */
	if (hServer != WTS_CURRENT_SERVER_HANDLE) return FALSE;

	/* Execute session manager RPC. */
	try
	{
		bSuccess = gClient->shutdownSystem(gAuthToken, ShutdownFlag);
	}
	catch (...)
	{
		bSuccess = FALSE;
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
	return FALSE;
}

HANDLE WINAPI
FreeRDS_WTSVirtualChannelOpen(
	HANDLE hServer,
	DWORD SessionId,
	LPSTR pVirtualName
)
{
	CHECK_CLIENT_CONNECTION();

	std::string result;
	std::string virtualName(pVirtualName);
	gClient->virtualChannelOpen(result, gAuthToken, SessionId, virtualName);

	if (result.size() == 0)
	{
		return NULL;
	}
	else
	{
#if 0
		HANDLE hNamedPipe;

		if (!WaitNamedPipeA(result.c_str(), 5000))
		{
			fprintf(stderr, "WaitNamedPipe failure: %s\n", result.c_str());
			return NULL;
		}

		hNamedPipe = CreateFileA(result.c_str(),
				GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE))
		{
			fprintf(stderr, "Failed to create named pipe %s\n", result.c_str());
			return NULL;
		}
		// add the sessionID and the virtual Name into a map
		// so we have this information for RpcVirtualChannelClose

		return hNamedPipe;
#endif
	}

	return NULL;
}

HANDLE WINAPI
FreeRDS_WTSVirtualChannelOpenEx(
	DWORD SessionId,
	LPSTR pVirtualName,
	DWORD flags
)
{
	return NULL;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelClose(
	HANDLE hChannelHandle
)
{
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
	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelPurgeInput(
	HANDLE hChannelHandle
)
{
	return TRUE;
}

BOOL WINAPI
FreeRDS_WTSVirtualChannelPurgeOutput(
	HANDLE hChannelHandle
)
{
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
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSUnRegisterSessionNotification(
	HWND hWnd
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSRegisterSessionNotificationEx(
	HANDLE hServer,
	HWND hWnd,
	DWORD dwFlags
)
{
	return FALSE;
}

BOOL WINAPI
FreeRDS_WTSUnRegisterSessionNotificationEx(
	HANDLE hServer,
	HWND hWnd
)
{
	return FALSE;
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
	int authStatus;

	CHECK_CLIENT_CONNECTION();

	/* Check parameters. */
	if (!CheckSessionId((LPDWORD)&SessionId)) return -1;

	/* Execute session manager RPC. */
	try
	{
		authStatus = gClient->authenticateUser(gAuthToken, SessionId, Username, Password, Domain);
	}
	catch (...)
	{
		authStatus = -1;
	}

	return authStatus;
}


