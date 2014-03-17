namespace cpp freerds
namespace java freerds
namespace php freerds
namespace perl freerds


typedef bool TBOOL
typedef i16 TINT16
typedef i32 TINT32
typedef string TSTRING

struct TClientDisplay
{
	1:TINT32 displayWidth;
	2:TINT32 displayHeight;
	3:TINT32 colorDepth;
}

struct TSessionInfo
{
	1:TINT32 sessionId;
	2:TINT32 connectState;
	3:TSTRING winStationName;
}

typedef list<TSessionInfo> TSessionInfoList

union TSessionInfoValue
{
	1:TBOOL boolValue;
	2:TINT16 int16Value;
	3:TINT32 int32Value;
	4:TSTRING stringValue;
	5:TClientDisplay displayValue;
}

struct TReturnEnumerateSessions
{
	1:TBOOL returnValue;
	2:TSessionInfoList sessionInfoList;
}

struct TReturnGetSessionInformation
{
	1:TBOOL returnValue;
	2:TSessionInfoValue infoValue;
}


service fdsapi {
	TINT32 ping(1:TINT32 input);

	/* ICPS protocol */
	TINT32 authenticateUser(1:TSTRING authToken, 2:TINT32 sessionId, 3:TSTRING username, 4:TSTRING password, 5:TSTRING domain);

	/* WTS protocol */
	TSTRING virtualChannelOpen(1:TSTRING authToken, 2:TINT32 sessionId, 3:TSTRING virtualName);
	TSTRING virtualChannelOpenEx(1:TSTRING authToken, 2:TINT32 sessionId, 3:TSTRING virtualName, 4:TINT32 flags);
	TBOOL virtualChannelClose(1:TSTRING authToken, 2:TINT32 sessionId, 3:TSTRING virtualName);
	TBOOL disconnectSession(1:TSTRING authToken, 2:TINT32 sessionId, 3:TBOOL wait);
	TBOOL logoffSession(1:TSTRING authToken, 2:TINT32 sessionId, 3:TBOOL wait);
	TBOOL shutdownSystem(1:TSTRING authToken, 2:TINT32 shutdownFlag);
	TReturnEnumerateSessions enumerateSessions(1:TSTRING authToken, 2:TINT32 Version);
	TReturnGetSessionInformation getSessionInformation(1:TSTRING authToken, 2:TINT32 sessionId, 3:TINT32 infoClass);
	TBOOL setSessionInformation(1:TSTRING authToken, 2:TINT32 sessionId, 3:TINT32 infoClass, 4:TSessionInfoValue infoValue);
}

