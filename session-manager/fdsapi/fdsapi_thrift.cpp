/**
 * FreeRDS: FDSApi implementation for thrift
 *
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
#include <winpr/crt.h>
#include <winpr/pipe.h>
#include <fdsapi/fdsapi.h>


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

int connectClient()
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

BOOL RpcStartRemoteControlSession(void* context, LPWSTR pTargetServerName,
		ULONG TargetLogonId, BYTE HotkeyVk, USHORT HotkeyModifiers)
{
	return TRUE;
}

BOOL RpcStopRemoteControlSession(void* context, ULONG LogonId)
{
	return TRUE;
}

BOOL RpcConnectSession(void* context, ULONG LogonId, ULONG TargetLogonId, PWSTR pPassword, BOOL bWait)
{
	return TRUE;
}

BOOL RpcEnumerateServers(void* context, LPWSTR pDomainName, DWORD Reserved,
		DWORD Version, PWTS_SERVER_INFOW* ppServerInfo, DWORD* pCount)
{
	return TRUE;
}

HANDLE RpcOpenServer(void* context, LPWSTR pServerName)
{
	return NULL;
}

HANDLE RpcOpenServerEx(void* context, LPWSTR pServerName)
{
	return NULL;
}

VOID RpcCloseServer(void* context, HANDLE hServer)
{

}

BOOL RpcEnumerateSessions(void* context, HANDLE hServer, DWORD Reserved,
		DWORD Version, PWTS_SESSION_INFOW* ppSessionInfo, DWORD* pCount)
{
	return TRUE;
}

BOOL RpcEnumerateSessionsEx(void* context, HANDLE hServer, DWORD* pLevel,
		DWORD Filter, PWTS_SESSION_INFO_1W* ppSessionInfo, DWORD* pCount)
{
	return TRUE;
}

BOOL RpcEnumerateProcesses(void* context, HANDLE hServer, DWORD Reserved,
		DWORD Version, PWTS_PROCESS_INFOW* ppProcessInfo, DWORD* pCount)
{
	return TRUE;
}

BOOL RpcTerminateProcess(void* context, HANDLE hServer, DWORD ProcessId, DWORD ExitCode)
{
	return TRUE;
}

BOOL RpcQuerySessionInformation(void* context, HANDLE hServer, DWORD SessionId,
		WTS_INFO_CLASS WTSInfoClass, LPWSTR* ppBuffer, DWORD* pBytesReturned)
{
	return TRUE;
}

BOOL RpcQueryUserConfig(void* context, LPWSTR pServerName, LPWSTR pUserName,
		WTS_CONFIG_CLASS WTSConfigClass, LPWSTR* ppBuffer, DWORD* pBytesReturned)
{
	return TRUE;
}

BOOL RpcSetUserConfig(void* context, LPWSTR pServerName, LPWSTR pUserName,
		WTS_CONFIG_CLASS WTSConfigClass, LPWSTR pBuffer, DWORD DataLength)
{
	return TRUE;
}

BOOL RpcSendMessage(void* context, HANDLE hServer, DWORD SessionId, LPWSTR pTitle, DWORD TitleLength,
		LPWSTR pMessage, DWORD MessageLength, DWORD Style, DWORD Timeout, DWORD* pResponse, BOOL bWait)
{
	return TRUE;
}

BOOL RpcDisconnectSession(void* context, HANDLE hServer, DWORD SessionId, BOOL bWait)
{
	return TRUE;
}

BOOL RpcLogoffSession(void* context, HANDLE hServer, DWORD SessionId, BOOL bWait)
{
	return TRUE;
}

BOOL RpcShutdownSystem(void* context, HANDLE hServer, DWORD ShutdownFlag)
{
	return TRUE;
}

BOOL RpcRegisterSessionNotification(void* context, HWND hWnd, DWORD dwFlags)
{
	return TRUE;
}

BOOL RpcUnRegisterSessionNotification(void* context, HWND hWnd)
{
	return TRUE;
}

BOOL RpcRegisterSessionNotificationEx(void* context, HANDLE hServer, HWND hWnd, DWORD dwFlags)
{
	return TRUE;
}

BOOL RpcUnRegisterSessionNotificationEx(void* context, HANDLE hServer, HWND hWnd)
{
	return TRUE;
}

BOOL RpcEnableChildSessions(void* context, BOOL bEnable)
{
	return TRUE;
}

BOOL RpcIsChildSessionsEnabled(void* context, PBOOL pbEnabled)
{
	return TRUE;
}

BOOL RpcGetChildSessionId(void* context, PULONG pSessionId)
{
	return TRUE;
}

HANDLE RpcVirtualChannelOpen(DWORD sessionId, LPSTR pVirtualName) {
	CHECK_CLIENT_CONNECTION();
	std::string result;
	std::string virtualName(pVirtualName);
	gClient->virtualChannelOpen(result,gAuthToken,sessionId,virtualName);
	if (result.size() == 0) {
		return NULL;
	} else {
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
	}
}

HANDLE RpcVirtualChannelOpenEx(DWORD SessionId, LPSTR pVirtualName, DWORD flags) {
	CHECK_CLIENT_CONNECTION();
	return NULL;
}
BOOL RpcVirtualChannelClose(HANDLE virtualChannelHandle) {
	CHECK_CLIENT_CONNECTION();
	//std::string virtualName(pVirtualName);
	//return gClient->virtualChannelClose(gAuthToken,SessionId,virtualName);
}


WTSFunctionTable FDSApiFunctionTable =
{
	1, /* dwVersion */
	RpcStartRemoteControlSession,
	RpcStopRemoteControlSession,
	RpcConnectSession,
	RpcEnumerateServers,
	RpcOpenServer,
	RpcOpenServerEx,
	RpcCloseServer,
	RpcEnumerateSessions,
	RpcEnumerateSessionsEx,
	RpcEnumerateProcesses,
	RpcTerminateProcess,
	RpcQuerySessionInformation,
	RpcQueryUserConfig,
	RpcSetUserConfig,
	RpcSendMessage,
	RpcDisconnectSession,
	RpcLogoffSession,
	RpcShutdownSystem,
	RpcRegisterSessionNotification,
	RpcUnRegisterSessionNotification,
	RpcRegisterSessionNotificationEx,
	RpcUnRegisterSessionNotificationEx,
	RpcEnableChildSessions,
	RpcIsChildSessionsEnabled,
	RpcGetChildSessionId,
	RpcVirtualChannelOpen,
	RpcVirtualChannelOpenEx,
	RpcVirtualChannelClose
};

PWTSFunctionTable FDSApiEntry() {
	return &FDSApiFunctionTable;
}
