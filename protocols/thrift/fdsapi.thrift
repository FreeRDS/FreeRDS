namespace cpp freerds
namespace java freerds
namespace php freerds
namespace perl freerds


typedef string TLPSTR
typedef i32 TDWORD

enum TWTS_CONNECTSTATE_CLASS
{
	WTSActive = 1,
	WTSConnected = 2,
	WTSConnectQuery = 3,
	WTSShadow = 4,
	WTSDisconnected = 5, 
	WTSIdle = 6,
	WTSListen = 7,
	WTSReset = 8,
	WTSDown = 9,
	WTSInit = 10,
}

struct TWTS_SESSION_INFOA
{
	1:TDWORD SessionId;
	2:TLPSTR pWinStationName;
	3:TWTS_CONNECTSTATE_CLASS State;
}

typedef list<TWTS_SESSION_INFOA> sessionList

struct ReturnEnumerateSession {
	1: bool returnValue;
	2:sessionList sessionInfoList;
}


service fdsapi {
	TDWORD ping(1:TDWORD input);
	TLPSTR virtualChannelOpen(1:TLPSTR authToken, 2:TDWORD sessionId,3:TLPSTR virtualName);
	TLPSTR virtualChannelOpenEx(1:TLPSTR authToken, 2:TDWORD sessionId,3:TLPSTR virtualName,4:TDWORD flags);
	bool virtualChannelClose(1:TLPSTR authToken,2:TDWORD sessionId, 3:TLPSTR virtualName);
	bool disconnectSession(1:TLPSTR authToken, 2:TDWORD sessionId, 3:bool wait);
	bool logoffSession(1:TLPSTR authToken,2:TDWORD sessionId, 3:bool wait);
	ReturnEnumerateSession enumerateSessions(1:TLPSTR authToken,2:TDWORD Version);
}


/**
typedef char * (*fpRpcVirtualChannelOpen)(DWORD SessionId, LPSTR pVirtualName);
typedef char * (*fpRpcVirtualChannelOpenEx)(DWORD SessionId, LPSTR pVirtualName, DWORD flags);
typedef BOOL (*fpRpcVirtualChannelClose)(DWORD SessionId, LPSTR pVirtualName);


typedef BOOL (*fpRpcStartRemoteControlSession)(void* context, LPWSTR pTargetServerName,ULONG TargetLogonId, BYTE HotkeyVk, USHORT HotkeyModifiers);
typedef BOOL (*fpRpcStopRemoteControlSession)(void* context, ULONG LogonId);
typedef BOOL (*fpRpcConnectSession)(void* context, ULONG LogonId, ULONG TargetLogonId, PWSTR pPassword, BOOL bWait);
typedef BOOL (*fpRpcEnumerateServers)(void* context, LPWSTR pDomainName, DWORD Reserved,DWORD Version, PWTS_SERVER_INFOW* ppServerInfo, DWORD* pCount);

typedef HANDLE (*fpRpcOpenServer)(void* context, LPWSTR pServerName);
typedef HANDLE (*fpRpcOpenServerEx)(void* context, LPWSTR pServerName);
typedef VOID (*fpRpcCloseServer)(void* context, HANDLE hServer);

typedef BOOL (*fpRpcEnumerateSessions)(void* context, HANDLE hServer, DWORD Reserved,DWORD Version, PWTS_SESSION_INFOW* ppSessionInfo, DWORD* pCount);
typedef BOOL (*fpRpcEnumerateSessionsEx)(void* context, HANDLE hServer, DWORD* pLevel,DWORD Filter, PWTS_SESSION_INFO_1W* ppSessionInfo, DWORD* pCount);

typedef BOOL (*fpRpcEnumerateProcesses)(void* context, HANDLE hServer, DWORD Reserved,DWORD Version, PWTS_PROCESS_INFOW* ppProcessInfo, DWORD* pCount);
typedef BOOL (*fpRpcTerminateProcess)(void* context, HANDLE hServer, DWORD ProcessId, DWORD ExitCode);

typedef BOOL (*fpRpcQuerySessionInformation)(void* context, HANDLE hServer, DWORD SessionId,WTS_INFO_CLASS WTSInfoClass, LPWSTR* ppBuffer, DWORD* pBytesReturned);

typedef BOOL (*fpRpcQueryUserConfig)(void* context, LPWSTR pServerName, LPWSTR pUserName,WTS_CONFIG_CLASS WTSConfigClass, LPWSTR* ppBuffer, DWORD* pBytesReturned);

typedef BOOL (*fpRpcSetUserConfig)(void* context, LPWSTR pServerName, LPWSTR pUserName,WTS_CONFIG_CLASS WTSConfigClass, LPWSTR pBuffer, DWORD DataLength);

typedef BOOL (*fpRpcSendMessage)(void* context, HANDLE hServer, DWORD SessionId, LPWSTR pTitle, DWORD TitleLength,LPWSTR pMessage, DWORD MessageLength, DWORD Style, DWORD Timeout, DWORD* pResponse, BOOL bWait);

typedef BOOL (*fpRpcDisconnectSession)(void* context, HANDLE hServer, DWORD SessionId, BOOL bWait);

typedef BOOL (*fpRpcLogoffSession)(void* context, HANDLE hServer, DWORD SessionId, BOOL bWait);

typedef BOOL (*fpRpcShutdownSystem)(void* context, HANDLE hServer, DWORD ShutdownFlag);

typedef BOOL (*fpRpcRegisterSessionNotification)(void* context, HWND hWnd, DWORD dwFlags);
typedef BOOL (*fpRpcUnRegisterSessionNotification)(void* context, HWND hWnd);

typedef BOOL (*fpRpcRegisterSessionNotificationEx)(void* context, HANDLE hServer, HWND hWnd, DWORD dwFlags);
typedef BOOL (*fpRpcUnRegisterSessionNotificationEx)(void* context, HANDLE hServer, HWND hWnd);

typedef BOOL (*fpRpcEnableChildSessions)(void* context, BOOL bEnable);
typedef BOOL (*fpRpcIsChildSessionsEnabled)(void* context, PBOOL pbEnabled);
typedef BOOL (*fpRpcGetChildSessionId)(void* context, PULONG pSessionId);
*/
