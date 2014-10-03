/**
 * FDSApiMessages
 *
 * Protocol definition for FDSAPI client/server interaction.
 *
 * Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
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

#ifndef FDSAPI_MESSAGES_H
#define FDSAPI_MESSAGES_H

#include <winpr/stream.h>

#define FDSAPI_PING_REQUEST_ID					1
#define FDSAPI_PING_RESPONSE_ID					2
#define FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST_ID			3
#define FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE_ID			4
#define FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST_ID		5
#define FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE_ID		6
#define FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST_ID			7
#define FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE_ID		8
#define FDSAPI_DISCONNECT_SESSION_REQUEST_ID			9
#define FDSAPI_DISCONNECT_SESSION_RESPONSE_ID			10
#define FDSAPI_LOGOFF_SESSION_REQUEST_ID			11
#define FDSAPI_LOGOFF_SESSION_RESPONSE_ID			12
#define FDSAPI_SHUTDOWN_SYSTEM_REQUEST_ID			13
#define FDSAPI_SHUTDOWN_SYSTEM_RESPONSE_ID			14
#define FDSAPI_ENUMERATE_SESSIONS_REQUEST_ID			15
#define FDSAPI_ENUMERATE_SESSIONS_RESPONSE_ID			16
#define FDSAPI_QUERY_SESSION_INFORMATION_REQUEST_ID		17
#define FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE_ID		18
#define FDSAPI_AUTHENTICATE_USER_REQUEST_ID			19
#define FDSAPI_AUTHENTICATE_USER_RESPONSE_ID			20

#define FDSAPI_SESSION_EVENT_ID					50

/**
 * TODO: For now, we only define a single message which represents an event
 * indicating a session state change.  These messages are delivered from the
 * server in an unsolicited manner.  Apache Thrift does not support this type
 * of messaging protocol.  Everything with Thrift involves a true RPC whereby
 * the client makes a call and expects a return.  The problem with the Thrift
 * model is that there exists the potential to miss a session event.
 *
 * In the next iteration, we can potentially replace Thrift altogether.
 */

typedef struct
{
	UINT32 sessionId;
	UINT32 connectState;
	const char* winStationName;
}
FDSAPI_SESSION_INFO;

typedef enum
{
	FDSAPI_SESSION_INFO_VALUE_BOOL,
	FDSAPI_SESSION_INFO_VALUE_UINT16,
	FDSAPI_SESSION_INFO_VALUE_UINT32,
	FDSAPI_SESSION_INFO_VALUE_STRING,
	FDSAPI_SESSION_INFO_VALUE_DISPLAY
}
FDSAPI_SESSION_INFO_VALUE_TYPE;

typedef struct
{
	FDSAPI_SESSION_INFO_VALUE_TYPE t;
	union {
		BOOL boolValue;
		UINT16 uint16Value;
		UINT32 uint32Value;
		const char* stringValue;
		struct {
			UINT32 displayWidth;
			UINT32 displayHeight;
			UINT32 colorDepth;
		} displayValue;
	} u;
}
FDSAPI_SESSION_INFO_VALUE;

typedef struct
{
	UINT32 input;
}
FDSAPI_PING_REQUEST;

typedef struct
{
	UINT32 result;
}
FDSAPI_PING_RESPONSE;

typedef struct
{
	UINT32 sessionId;
	const char* virtualName;
}
FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST;

typedef struct
{
	const char* endPoint;
}
FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE;

typedef struct
{
	UINT32 sessionId;
	const char* virtualName;
	UINT32 flags;
}
FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST;

typedef struct
{
	const char* endPoint;
}
FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE;

typedef struct
{
	UINT32 sessionId;
	const char* virtualName;
}
FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST;

typedef struct
{
	BOOL result;
}
FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE;

typedef struct
{
	UINT32 sessionId;
	BOOL wait;
}
FDSAPI_DISCONNECT_SESSION_REQUEST;

typedef struct
{
	BOOL result;
}
FDSAPI_DISCONNECT_SESSION_RESPONSE;

typedef struct
{
	UINT32 sessionId;
	BOOL wait;
}
FDSAPI_LOGOFF_SESSION_REQUEST;

typedef struct
{
	BOOL result;
}
FDSAPI_LOGOFF_SESSION_RESPONSE;

typedef struct
{
	UINT32 shutdownFlag;
}
FDSAPI_SHUTDOWN_SYSTEM_REQUEST;

typedef struct
{
	BOOL result;
}
FDSAPI_SHUTDOWN_SYSTEM_RESPONSE;

typedef struct
{
	UINT32 version;
}
FDSAPI_ENUMERATE_SESSIONS_REQUEST;

typedef struct
{
	BOOL result;
	UINT32 cSessions;
	FDSAPI_SESSION_INFO* pSessionInfo;
}
FDSAPI_ENUMERATE_SESSIONS_RESPONSE;

typedef struct
{
	UINT32 sessionId;
	UINT32 infoClass;
}
FDSAPI_QUERY_SESSION_INFORMATION_REQUEST;

typedef struct
{
	BOOL result;
	FDSAPI_SESSION_INFO_VALUE infoValue;
}
FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE;

typedef struct
{
	UINT32 sessionId;
	const char* username;
	const char* password;
	const char* domain;
}
FDSAPI_AUTHENTICATE_USER_REQUEST;

typedef struct
{
	UINT32 result;
}
FDSAPI_AUTHENTICATE_USER_RESPONSE;


/**
 *
 * FDSAPI_SESSION_EVENT
 *
 * Broadcast from the server to all clients whenever a session undergoes a
 * state change (e.g., WTSActive to WTSDisconnected).  The client uses this
 * message to complete a call to WTSWaitSystemEvent or to notify the client
 * process of session events that have been registered for via a call to
 * WTSRegisterNotification.
 *
 */
typedef struct
{
	UINT32 sessionId;
	UINT32 stateChange;
}
FDSAPI_SESSION_EVENT;

typedef struct
{
	UINT16 messageId;
	UINT16 requestId;
	union
	{
		/* Request/Response pairs */
		FDSAPI_PING_REQUEST pingRequest;
		FDSAPI_PING_RESPONSE pingResponse;
		FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST virtualChannelOpenRequest;
		FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE virtualChannelOpenResponse;
		FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST virtualChannelOpenExRequest;
		FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE virtualChannelOpenExResponse;
		FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST virtualChannelCloseRequest;
		FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE virtualChannelCloseResponse;
		FDSAPI_DISCONNECT_SESSION_REQUEST disconnectSessionRequest;
		FDSAPI_DISCONNECT_SESSION_RESPONSE disconnectSessionResponse;
		FDSAPI_LOGOFF_SESSION_REQUEST logoffSessionRequest;
		FDSAPI_LOGOFF_SESSION_RESPONSE logoffSessionResponse;
		FDSAPI_SHUTDOWN_SYSTEM_REQUEST shutdownSystemRequest;
		FDSAPI_SHUTDOWN_SYSTEM_RESPONSE shutdownSystemResponse;
		FDSAPI_ENUMERATE_SESSIONS_REQUEST enumerateSessionsRequest;
		FDSAPI_ENUMERATE_SESSIONS_RESPONSE enumerateSessionsResponse;
		FDSAPI_QUERY_SESSION_INFORMATION_REQUEST querySessionInformationRequest;
		FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE querySessionInformationResponse;
		FDSAPI_AUTHENTICATE_USER_REQUEST authenticateUserRequest;
		FDSAPI_AUTHENTICATE_USER_RESPONSE authenticateUserResponse;

		/* Events */
		FDSAPI_SESSION_EVENT sessionEvent;
	} u;
}
FDSAPI_MESSAGE;

/**
 * Encoder/decoders
 */

#ifdef __cplusplus
extern "C" {
#endif

BOOL FDSAPI_DecodeMessage(wStream* s, FDSAPI_MESSAGE* msg);
wStream* FDSAPI_EncodeMessage(FDSAPI_MESSAGE* msg);
void FDSAPI_FreeMessage(FDSAPI_MESSAGE* msg);

#ifdef __cplusplus
}
#endif

#endif
