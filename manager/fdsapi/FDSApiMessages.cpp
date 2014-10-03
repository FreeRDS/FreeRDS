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

#include <winpr/crt.h>

#include "FDSApiMessages.h"


/* Base types */

#define FDSAPI_SizeOfBOOL()		1
#define FDSAPI_SizeOfUINT8()	1
#define FDSAPI_SizeOfUINT16()	2
#define FDSAPI_SizeOfUINT32()	4

static BOOL FDSAPI_DecodeBOOL(wStream* s, BOOL* boolValue)
{
	UINT8 uint8Value;

	if (Stream_GetRemainingLength(s) < 1) return FALSE;

	Stream_Read_UINT8(s, uint8Value);

	*boolValue = (uint8Value ? TRUE : FALSE);

	return TRUE;
}

static void FDSAPI_EncodeBOOL(wStream* s, BOOL boolValue)
{
	Stream_Write_UINT8(s, boolValue ? 1 : 0);
}

static BOOL FDSAPI_DecodeUINT8(wStream* s, UINT8* uint8Value)
{
	if (Stream_GetRemainingLength(s) < 1) return FALSE;

	Stream_Read_UINT8(s, *uint8Value);

	return TRUE;
}

static void FDSAPI_EncodeUINT8(wStream* s, UINT8 uint8Value)
{
	Stream_Write_UINT8(s, uint8Value);
}

static BOOL FDSAPI_DecodeUINT16(wStream* s, UINT16* uint16Value)
{
	if (Stream_GetRemainingLength(s) < 2) return FALSE;

	Stream_Read_UINT16(s, *uint16Value);

	return TRUE;
}

static void FDSAPI_EncodeUINT16(wStream* s, UINT16 uint16Value)
{
	Stream_Write_UINT16(s, uint16Value);
}

static BOOL FDSAPI_DecodeUINT32(wStream* s, UINT32* uint32Value)
{
	if (Stream_GetRemainingLength(s) < 4) return FALSE;

	Stream_Read_UINT32(s, *uint32Value);

	return TRUE;
}

static void FDSAPI_EncodeUINT32(wStream* s, UINT32 uint32Value)
{
	Stream_Write_UINT32(s, uint32Value);
}

static UINT32 FDSAPI_SizeOfString(const char* stringValue)
{
	return 2 + (stringValue ? strlen(stringValue) : 0);
}

static BOOL FDSAPI_DecodeString(wStream* s, const char** stringValue)
{
	char* mutableString;
	UINT16 stringSize;

	if (Stream_GetRemainingLength(s) < 2) return FALSE;

	Stream_Read_UINT16(s, stringSize);

	*stringValue = NULL;

	if (stringSize > 0)
	{
		if (Stream_GetRemainingLength(s) < stringSize) return FALSE;

		mutableString = (char*)malloc(stringSize + 1);
		if (mutableString == NULL) return FALSE;

		Stream_Read(s, mutableString, stringSize);
		mutableString[stringSize] = '\0';

		*stringValue = (const char*)mutableString;
	}

	return TRUE;
}

static void FDSAPI_EncodeString(wStream* s, const char* stringValue)
{
	UINT16 stringSize = (stringValue ? strlen(stringValue) : 0);

	Stream_Write_UINT16(s, stringSize);

	if (stringSize > 0)
	{
		Stream_Write(s, stringValue, stringSize);
	}
}

static void FDSAPI_FreeString(const char *stringValue)
{
	if (stringValue)
	{
		free((void*)stringValue);
	}
}


/* PING */
static UINT32 FDSAPI_SizeOfPingRequest(FDSAPI_PING_REQUEST* pingRequest)
{
	return FDSAPI_SizeOfUINT32();
}

static BOOL FDSAPI_DecodePingRequest(wStream* s, FDSAPI_PING_REQUEST* pingRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &pingRequest->input)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodePingRequest(wStream *s, FDSAPI_PING_REQUEST* pingRequest)
{
	FDSAPI_EncodeUINT32(s, pingRequest->input);
}

static void FDSAPI_FreePingRequest(FDSAPI_PING_REQUEST* pingRequest)
{
}

static UINT32 FDSAPI_SizeOfPingResponse(FDSAPI_PING_RESPONSE* pingResponse)
{
	return FDSAPI_SizeOfUINT32();
}

static BOOL FDSAPI_DecodePingResponse(wStream* s, FDSAPI_PING_RESPONSE* pingResponse)
{
	if (!FDSAPI_DecodeUINT32(s, &pingResponse->result)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodePingResponse(wStream* s, FDSAPI_PING_RESPONSE* pingResponse)
{
	FDSAPI_EncodeUINT32(s, pingResponse->result);
}

static void FDSAPI_FreePingResponse(FDSAPI_PING_RESPONSE* pingResponse)
{
}


/* VIRTUAL_CHANNEL_OPEN */

static UINT32 FDSAPI_SizeOfVirtualChannelOpenRequest(FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST* virtualChannelOpenRequest)
{
	return FDSAPI_SizeOfUINT32() + FDSAPI_SizeOfString(virtualChannelOpenRequest->virtualName);
}

static BOOL FDSAPI_DecodeVirtualChannelOpenRequest(wStream* s, FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST* virtualChannelOpenRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &virtualChannelOpenRequest->sessionId)) return FALSE;
	if (!FDSAPI_DecodeString(s, &virtualChannelOpenRequest->virtualName)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeVirtualChannelOpenRequest(wStream* s, FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST* virtualChannelOpenRequest)
{
	FDSAPI_EncodeUINT32(s, virtualChannelOpenRequest->sessionId);
	FDSAPI_EncodeString(s, virtualChannelOpenRequest->virtualName);
}

static void FDSAPI_FreeVirtualChannelOpenRequest(FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST* virtualChannelOpenRequest)
{
	FDSAPI_FreeString(virtualChannelOpenRequest->virtualName);
}

static UINT32 FDSAPI_SizeOfVirtualChannelOpenResponse(FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE* virtualChannelOpenResponse)
{
	return FDSAPI_SizeOfString(virtualChannelOpenResponse->endPoint);
}

static BOOL FDSAPI_DecodeVirtualChannelOpenResponse(wStream* s, FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE* virtualChannelOpenResponse)
{
	if (!FDSAPI_DecodeString(s, &virtualChannelOpenResponse->endPoint)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeVirtualChannelOpenResponse(wStream* s, FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE* virtualChannelOpenResponse)
{
	FDSAPI_EncodeString(s, virtualChannelOpenResponse->endPoint);
}

static void FDSAPI_FreeVirtualChannelOpenResponse(FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE* virtualChannelOpenResponse)
{
	FDSAPI_FreeString(virtualChannelOpenResponse->endPoint);
}


/* VIRTUAL_CHANNEL_OPEN_EX */

static UINT32 FDSAPI_SizeOfVirtualChannelOpenExRequest(FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST* virtualChannelOpenExRequest)
{
	return (2 * FDSAPI_SizeOfUINT32()) + FDSAPI_SizeOfString(virtualChannelOpenExRequest->virtualName);
}

static BOOL FDSAPI_DecodeVirtualChannelOpenExRequest(wStream* s, FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST* virtualChannelOpenExRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &virtualChannelOpenExRequest->sessionId)) return FALSE;
	if (!FDSAPI_DecodeString(s, &virtualChannelOpenExRequest->virtualName)) return FALSE;
	if (!FDSAPI_DecodeUINT32(s, &virtualChannelOpenExRequest->flags)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeVirtualChannelOpenExRequest(wStream* s, FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST* virtualChannelOpenExRequest)
{
	FDSAPI_EncodeUINT32(s, virtualChannelOpenExRequest->sessionId);
	FDSAPI_EncodeString(s, virtualChannelOpenExRequest->virtualName);
	FDSAPI_EncodeUINT32(s, virtualChannelOpenExRequest->flags);
}

static void FDSAPI_FreeVirtualChannelOpenExRequest(FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST* virtualChannelOpenExRequest)
{
	FDSAPI_FreeString(virtualChannelOpenExRequest->virtualName);
}

static UINT32 FDSAPI_SizeOfVirtualChannelOpenExResponse(FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE* virtualChannelOpenExResponse)
{
	return FDSAPI_SizeOfString(virtualChannelOpenExResponse->endPoint);
}

static BOOL FDSAPI_DecodeVirtualChannelOpenExResponse(wStream* s, FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE* virtualChannelOpenExResponse)
{
	if (!FDSAPI_DecodeString(s, &virtualChannelOpenExResponse->endPoint)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeVirtualChannelOpenExResponse(wStream* s, FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE* virtualChannelOpenExResponse)
{
	FDSAPI_EncodeString(s, virtualChannelOpenExResponse->endPoint);
}

static void FDSAPI_FreeVirtualChannelOpenExResponse(FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE* virtualChannelOpenExResponse)
{
	FDSAPI_FreeString(virtualChannelOpenExResponse->endPoint);
}


/* VIRTUAL_CHANNEL_CLOSE */

static UINT32 FDSAPI_SizeOfVirtualChannelCloseRequest(FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST* virtualChannelCloseRequest)
{
	return FDSAPI_SizeOfUINT32() + FDSAPI_SizeOfString(virtualChannelCloseRequest->virtualName);
}

static BOOL FDSAPI_DecodeVirtualChannelCloseRequest(wStream* s, FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST* virtualChannelCloseRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &virtualChannelCloseRequest->sessionId)) return FALSE;
	if (!FDSAPI_DecodeString(s, &virtualChannelCloseRequest->virtualName)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeVirtualChannelCloseRequest(wStream* s, FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST* virtualChannelCloseRequest)
{
	FDSAPI_EncodeUINT32(s, virtualChannelCloseRequest->sessionId);
	FDSAPI_EncodeString(s, virtualChannelCloseRequest->virtualName);
}

static void FDSAPI_FreeVirtualChannelCloseRequest(FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST* virtualChannelCloseRequest)
{
	FDSAPI_FreeString(virtualChannelCloseRequest->virtualName);
}

static UINT32 FDSAPI_SizeOfVirtualChannelCloseResponse(FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE* virtualChannelCloseResponse)
{
	return FDSAPI_SizeOfBOOL();
}

static BOOL FDSAPI_DecodeVirtualChannelCloseResponse(wStream* s, FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE* virtualChannelCloseResponse)
{
	if (!FDSAPI_DecodeBOOL(s, &virtualChannelCloseResponse->result)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeVirtualChannelCloseResponse(wStream* s, FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE* virtualChannelCloseResponse)
{
	FDSAPI_EncodeBOOL(s, virtualChannelCloseResponse->result);
}

static void FDSAPI_FreeVirtualChannelCloseResponse(FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE* virtualChannelCloseResponse)
{
}


/* DISCONNECT_SESSION */

static UINT32 FDSAPI_SizeOfDisconnectSessionRequest(FDSAPI_DISCONNECT_SESSION_REQUEST* disconnectSessionRequest)
{
	return FDSAPI_SizeOfUINT32() + FDSAPI_SizeOfBOOL();
}

static BOOL FDSAPI_DecodeDisconnectSessionRequest(wStream* s, FDSAPI_DISCONNECT_SESSION_REQUEST* disconnectSessionRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &disconnectSessionRequest->sessionId)) return FALSE;
	if (!FDSAPI_DecodeBOOL(s, &disconnectSessionRequest->wait)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeDisconnectSessionRequest(wStream *s, FDSAPI_DISCONNECT_SESSION_REQUEST* disconnectSessionRequest)
{
	FDSAPI_EncodeUINT32(s, disconnectSessionRequest->sessionId);
	FDSAPI_EncodeBOOL(s, disconnectSessionRequest->wait);
}

static void FDSAPI_FreeDisconnectSessionRequest(FDSAPI_DISCONNECT_SESSION_REQUEST* disconnectSessionRequest)
{
}

static UINT32 FDSAPI_SizeOfDisconnectSessionResponse(FDSAPI_DISCONNECT_SESSION_RESPONSE* disconnectSessionResponse)
{
	return FDSAPI_SizeOfBOOL();
}

static BOOL FDSAPI_DecodeDisconnectSessionResponse(wStream* s, FDSAPI_DISCONNECT_SESSION_RESPONSE* disconnectSessionResponse)
{
	if (!FDSAPI_DecodeBOOL(s, &disconnectSessionResponse->result)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeDisconnectSessionResponse(wStream* s, FDSAPI_DISCONNECT_SESSION_RESPONSE* disconnectSessionResponse)
{
	FDSAPI_EncodeBOOL(s, disconnectSessionResponse->result);
}

static void FDSAPI_FreeDisconnectSessionResponse(FDSAPI_DISCONNECT_SESSION_RESPONSE* disconnectSessionResponse)
{
}


/* LOGOFF_SESSION */

static UINT32 FDSAPI_SizeOfLogoffSessionRequest(FDSAPI_LOGOFF_SESSION_REQUEST* logoffSessionRequest)
{
	return FDSAPI_SizeOfUINT32() + FDSAPI_SizeOfBOOL();
}

static BOOL FDSAPI_DecodeLogoffSessionRequest(wStream* s, FDSAPI_LOGOFF_SESSION_REQUEST* logoffSessionRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &logoffSessionRequest->sessionId)) return FALSE;
	if (!FDSAPI_DecodeBOOL(s, &logoffSessionRequest->wait)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeLogoffSessionRequest(wStream *s, FDSAPI_LOGOFF_SESSION_REQUEST* logoffSessionRequest)
{
	FDSAPI_EncodeUINT32(s, logoffSessionRequest->sessionId);
	FDSAPI_EncodeBOOL(s, logoffSessionRequest->wait);
}

static void FDSAPI_FreeLogoffSessionRequest(FDSAPI_LOGOFF_SESSION_REQUEST* logoffSessionRequest)
{
}

static UINT32 FDSAPI_SizeOfLogoffSessionResponse(FDSAPI_LOGOFF_SESSION_RESPONSE* logoffSessionResponse)
{
	return FDSAPI_SizeOfBOOL();
}

static BOOL FDSAPI_DecodeLogoffSessionResponse(wStream* s, FDSAPI_LOGOFF_SESSION_RESPONSE* logoffSessionResponse)
{
	if (!FDSAPI_DecodeBOOL(s, &logoffSessionResponse->result)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeLogoffSessionResponse(wStream* s, FDSAPI_LOGOFF_SESSION_RESPONSE* logoffSessionResponse)
{
	FDSAPI_EncodeBOOL(s, logoffSessionResponse->result);
}

static void FDSAPI_FreeLogoffSessionResponse(FDSAPI_LOGOFF_SESSION_RESPONSE* logoffSessionResponse)
{
}


/* SHUTDOWN_SYSTEM */

static UINT32 FDSAPI_SizeOfShutdownSystemRequest(FDSAPI_SHUTDOWN_SYSTEM_REQUEST* shutdownSystemRequest)
{
	return FDSAPI_SizeOfUINT32();
}

static BOOL FDSAPI_DecodeShutdownSystemRequest(wStream* s, FDSAPI_SHUTDOWN_SYSTEM_REQUEST* shutdownSystemRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &shutdownSystemRequest->shutdownFlag)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeShutdownSystemRequest(wStream *s, FDSAPI_SHUTDOWN_SYSTEM_REQUEST* shutdownSystemRequest)
{
	FDSAPI_EncodeUINT32(s, shutdownSystemRequest->shutdownFlag);
}

static void FDSAPI_FreeShutdownSystemRequest(FDSAPI_SHUTDOWN_SYSTEM_REQUEST* shutdownSystemRequest)
{
}

static UINT32 FDSAPI_SizeOfShutdownSystemResponse(FDSAPI_SHUTDOWN_SYSTEM_RESPONSE* shutdownSystemResponse)
{
	return FDSAPI_SizeOfBOOL();
}

static BOOL FDSAPI_DecodeShutdownSystemResponse(wStream* s, FDSAPI_SHUTDOWN_SYSTEM_RESPONSE* shutdownSystemResponse)
{
	if (!FDSAPI_DecodeBOOL(s, &shutdownSystemResponse->result)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeShutdownSystemResponse(wStream* s, FDSAPI_SHUTDOWN_SYSTEM_RESPONSE* shutdownSystemResponse)
{
	FDSAPI_EncodeBOOL(s, shutdownSystemResponse->result);
}

static void FDSAPI_FreeShutdownSystemResponse(FDSAPI_SHUTDOWN_SYSTEM_RESPONSE* shutdownSystemResponse)
{
}


/* ENUMERATE_SESSIONS */

static UINT32 FDSAPI_SizeOfEnumerateSessionsRequest(FDSAPI_ENUMERATE_SESSIONS_REQUEST* enumerateSessionsRequest)
{
	return FDSAPI_SizeOfUINT32();
}

static BOOL FDSAPI_DecodeEnumerateSessionsRequest(wStream* s, FDSAPI_ENUMERATE_SESSIONS_REQUEST* enumerateSessionsRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &enumerateSessionsRequest->version)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeEnumerateSessionsRequest(wStream* s, FDSAPI_ENUMERATE_SESSIONS_REQUEST* enumerateSessionsRequest)
{
	FDSAPI_EncodeUINT32(s, enumerateSessionsRequest->version);
}

static void FDSAPI_FreeEnumerateSessionsRequest(FDSAPI_ENUMERATE_SESSIONS_REQUEST* enumerateSessionsRequest)
{
}

static UINT32 FDSAPI_SizeOfEnumerateSessionsResponse(FDSAPI_ENUMERATE_SESSIONS_RESPONSE* enumerateSessionsResponse)
{
	UINT32 size = FDSAPI_SizeOfBOOL() + FDSAPI_SizeOfUINT32();

	if (enumerateSessionsResponse->result)
	{
		UINT32 i;

		for (i = 0; i < enumerateSessionsResponse->cSessions; i++)
		{
			FDSAPI_SESSION_INFO* pSessionInfo = &enumerateSessionsResponse->pSessionInfo[i];

			size += (2 * FDSAPI_SizeOfUINT32()) + FDSAPI_SizeOfString(pSessionInfo->winStationName);
		}
	}

	return size;
}

static BOOL FDSAPI_DecodeEnumerateSessionsResponse(wStream* s, FDSAPI_ENUMERATE_SESSIONS_RESPONSE* enumerateSessionsResponse)
{
	if (!FDSAPI_DecodeBOOL(s, &enumerateSessionsResponse->result)) return FALSE;

	if (enumerateSessionsResponse->result)
	{
		if (!FDSAPI_DecodeUINT32(s, &enumerateSessionsResponse->cSessions)) return FALSE;

		if (enumerateSessionsResponse->cSessions > 0)
		{
			UINT32 i;

			enumerateSessionsResponse->pSessionInfo = (FDSAPI_SESSION_INFO*)
				malloc(enumerateSessionsResponse->cSessions * sizeof(FDSAPI_SESSION_INFO));
			if (enumerateSessionsResponse->pSessionInfo == NULL) return FALSE;

			for (i = 0; i < enumerateSessionsResponse->cSessions; i++)
			{
				FDSAPI_SESSION_INFO* pSessionInfo = &enumerateSessionsResponse->pSessionInfo[i];

				if (!FDSAPI_DecodeUINT32(s, &pSessionInfo->sessionId)) return FALSE;
				if (!FDSAPI_DecodeUINT32(s, &pSessionInfo->connectState)) return FALSE;
				if (!FDSAPI_DecodeString(s, &pSessionInfo->winStationName)) return FALSE;
			}
		}
	}

	return TRUE;
}

static void FDSAPI_EncodeEnumerateSessionsResponse(wStream* s, FDSAPI_ENUMERATE_SESSIONS_RESPONSE* enumerateSessionsResponse)
{
	FDSAPI_EncodeBOOL(s, enumerateSessionsResponse->result);

	if (enumerateSessionsResponse->result)
	{
		UINT32 i;

		FDSAPI_EncodeUINT32(s, enumerateSessionsResponse->cSessions);

		for (i = 0; i < enumerateSessionsResponse->cSessions; i++)
		{
			FDSAPI_SESSION_INFO* pSessionInfo = &enumerateSessionsResponse->pSessionInfo[i];

			FDSAPI_EncodeUINT32(s, pSessionInfo->sessionId);
			FDSAPI_EncodeUINT32(s, pSessionInfo->connectState);
			FDSAPI_EncodeString(s, pSessionInfo->winStationName);
		}
	}
}

static void FDSAPI_FreeEnumerateSessionsResponse(FDSAPI_ENUMERATE_SESSIONS_RESPONSE* enumerateSessionsResponse)
{
	UINT32 i;

	for (i = 0; i < enumerateSessionsResponse->cSessions; i++)
	{
		FDSAPI_SESSION_INFO* pSessionInfo = &enumerateSessionsResponse->pSessionInfo[i];

		FDSAPI_FreeString(pSessionInfo->winStationName);
	}

	free((void*)enumerateSessionsResponse->pSessionInfo);
}


/* QUERY_SESSION_INFORMATION */

static UINT32 FDSAPI_SizeOfQuerySessionInformationRequest(FDSAPI_QUERY_SESSION_INFORMATION_REQUEST* querySessionInformationRequest)
{
	return 2 * FDSAPI_SizeOfUINT32();
}

static BOOL FDSAPI_DecodeQuerySessionInformationRequest(wStream* s, FDSAPI_QUERY_SESSION_INFORMATION_REQUEST* querySessionInformationRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &querySessionInformationRequest->sessionId)) return FALSE;
	if (!FDSAPI_DecodeUINT32(s, &querySessionInformationRequest->infoClass)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeQuerySessionInformationRequest(wStream* s, FDSAPI_QUERY_SESSION_INFORMATION_REQUEST* querySessionInformationRequest)
{
	FDSAPI_EncodeUINT32(s, querySessionInformationRequest->sessionId);
	FDSAPI_EncodeUINT32(s, querySessionInformationRequest->infoClass);
}

static void FDSAPI_FreeQuerySessionInformationRequest(FDSAPI_QUERY_SESSION_INFORMATION_REQUEST* querySessionInformationRequest)
{
}

static UINT32 FDSAPI_SizeOfQuerySessionInformationResponse(FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE* querySessionInformationResponse)
{
	UINT32 size = FDSAPI_SizeOfUINT32();

	if (querySessionInformationResponse->result)
	{
		size += FDSAPI_SizeOfUINT8();

		switch (querySessionInformationResponse->infoValue.t)
		{
			case FDSAPI_SESSION_INFO_VALUE_BOOL:
				size += FDSAPI_SizeOfBOOL();
				break;

			case FDSAPI_SESSION_INFO_VALUE_UINT16:
				size += FDSAPI_SizeOfUINT16();
				break;

			case FDSAPI_SESSION_INFO_VALUE_UINT32:
				size += FDSAPI_SizeOfUINT32();
				break;

			case FDSAPI_SESSION_INFO_VALUE_STRING:
				size += FDSAPI_SizeOfString(querySessionInformationResponse->infoValue.u.stringValue);
				break;

			case FDSAPI_SESSION_INFO_VALUE_DISPLAY:
				size += (3 * FDSAPI_SizeOfUINT32());
				break;

			default:
				break;
		}
	}

	return size;
}

static BOOL FDSAPI_DecodeQuerySessionInformationResponse(wStream* s, FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE* querySessionInformationResponse)
{
	if (!FDSAPI_DecodeBOOL(s, &querySessionInformationResponse->result)) return FALSE;

	if (querySessionInformationResponse->result)
	{
		UINT16 uint16Value;

		if (!FDSAPI_DecodeUINT16(s, &uint16Value)) return FALSE;

		querySessionInformationResponse->infoValue.t = (FDSAPI_SESSION_INFO_VALUE_TYPE)uint16Value;

		switch (querySessionInformationResponse->infoValue.t)
		{
			case FDSAPI_SESSION_INFO_VALUE_BOOL:
				if (!FDSAPI_DecodeBOOL(s, &querySessionInformationResponse->infoValue.u.boolValue)) return FALSE;
				break;

			case FDSAPI_SESSION_INFO_VALUE_UINT16:
				if (!FDSAPI_DecodeUINT16(s, &querySessionInformationResponse->infoValue.u.uint16Value)) return FALSE;
				break;

			case FDSAPI_SESSION_INFO_VALUE_UINT32:
				if (!FDSAPI_DecodeUINT32(s, &querySessionInformationResponse->infoValue.u.uint32Value)) return FALSE;
				break;

			case FDSAPI_SESSION_INFO_VALUE_STRING:
				if (!FDSAPI_DecodeString(s, &querySessionInformationResponse->infoValue.u.stringValue)) return FALSE;
				break;

			case FDSAPI_SESSION_INFO_VALUE_DISPLAY:
				if (!FDSAPI_DecodeUINT32(s, &querySessionInformationResponse->infoValue.u.displayValue.displayWidth)) return FALSE;
				if (!FDSAPI_DecodeUINT32(s, &querySessionInformationResponse->infoValue.u.displayValue.displayHeight)) return FALSE;
				if (!FDSAPI_DecodeUINT32(s, &querySessionInformationResponse->infoValue.u.displayValue.colorDepth)) return FALSE;
				break;

			default:
				break;
		}
	}

	return TRUE;
}

static void FDSAPI_EncodeQuerySessionInformationResponse(wStream* s, FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE* querySessionInformationResponse)
{
	FDSAPI_EncodeBOOL(s, querySessionInformationResponse->result);

	if (querySessionInformationResponse->result)
	{
		UINT16 uint16Value = (UINT16)querySessionInformationResponse->infoValue.t;

		FDSAPI_EncodeUINT16(s, uint16Value);

		switch (querySessionInformationResponse->infoValue.t)
		{
			case FDSAPI_SESSION_INFO_VALUE_BOOL:
				FDSAPI_EncodeBOOL(s, querySessionInformationResponse->infoValue.u.boolValue);
				break;

			case FDSAPI_SESSION_INFO_VALUE_UINT16:
				FDSAPI_EncodeUINT16(s, querySessionInformationResponse->infoValue.u.uint16Value);
				break;

			case FDSAPI_SESSION_INFO_VALUE_UINT32:
				FDSAPI_EncodeUINT32(s, querySessionInformationResponse->infoValue.u.uint32Value);
				break;

			case FDSAPI_SESSION_INFO_VALUE_STRING:
				FDSAPI_EncodeString(s, querySessionInformationResponse->infoValue.u.stringValue);
				break;

			case FDSAPI_SESSION_INFO_VALUE_DISPLAY:
				FDSAPI_EncodeUINT32(s, querySessionInformationResponse->infoValue.u.displayValue.displayWidth);
				FDSAPI_EncodeUINT32(s, querySessionInformationResponse->infoValue.u.displayValue.displayHeight);
				FDSAPI_EncodeUINT32(s, querySessionInformationResponse->infoValue.u.displayValue.colorDepth);
				break;

			default:
				break;
		}
	}
}

static void FDSAPI_FreeQuerySessionInformationResponse(FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE* querySessionInformationResponse)
{
	if (querySessionInformationResponse->result)
	{
		switch (querySessionInformationResponse->infoValue.t)
		{
			case FDSAPI_SESSION_INFO_VALUE_STRING:
				FDSAPI_FreeString(querySessionInformationResponse->infoValue.u.stringValue);
				break;

			default:
				break;
		}
	}
}


/* AUTHENTICATE_USER */

static UINT32 FDSAPI_SizeOfAuthenticateUserRequest(FDSAPI_AUTHENTICATE_USER_REQUEST* authenticateUserRequest)
{
	return FDSAPI_SizeOfUINT32() +
		FDSAPI_SizeOfString(authenticateUserRequest->username) +
		FDSAPI_SizeOfString(authenticateUserRequest->password) +
		FDSAPI_SizeOfString(authenticateUserRequest->domain);
}

static BOOL FDSAPI_DecodeAuthenticateUserRequest(wStream* s, FDSAPI_AUTHENTICATE_USER_REQUEST* authenticateUserRequest)
{
	if (!FDSAPI_DecodeUINT32(s, &authenticateUserRequest->sessionId)) return FALSE;
	if (!FDSAPI_DecodeString(s, &authenticateUserRequest->username)) return FALSE;
	if (!FDSAPI_DecodeString(s, &authenticateUserRequest->password)) return FALSE;
	if (!FDSAPI_DecodeString(s, &authenticateUserRequest->domain)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeAuthenticateUserRequest(wStream* s, FDSAPI_AUTHENTICATE_USER_REQUEST* authenticateUserRequest)
{
	FDSAPI_EncodeUINT32(s, authenticateUserRequest->sessionId);
	FDSAPI_EncodeString(s, authenticateUserRequest->username);
	FDSAPI_EncodeString(s, authenticateUserRequest->password);
	FDSAPI_EncodeString(s, authenticateUserRequest->domain);
}

static void FDSAPI_FreeAuthenticateUserRequest(FDSAPI_AUTHENTICATE_USER_REQUEST* authenticateUserRequest)
{
	FDSAPI_FreeString(authenticateUserRequest->username);
	FDSAPI_FreeString(authenticateUserRequest->password);
	FDSAPI_FreeString(authenticateUserRequest->domain);
}

static UINT32 FDSAPI_SizeOfAuthenticateUserResponse(FDSAPI_AUTHENTICATE_USER_RESPONSE* authenticateUserResponse)
{
	return FDSAPI_SizeOfUINT32();
}

static BOOL FDSAPI_DecodeAuthenticateUserResponse(wStream* s, FDSAPI_AUTHENTICATE_USER_RESPONSE* authenticateUserResponse)
{
	if (!FDSAPI_DecodeUINT32(s, &authenticateUserResponse->result)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeAuthenticateUserResponse(wStream* s, FDSAPI_AUTHENTICATE_USER_RESPONSE* authenticateUserResponse)
{
	FDSAPI_EncodeUINT32(s, authenticateUserResponse->result);
}

static void FDSAPI_FreeAuthenticateUserResponse(FDSAPI_AUTHENTICATE_USER_RESPONSE* authenticateUserResponse)
{
}


/* SESSION_EVENT */

static UINT32 FDSAPI_SizeOfSessionEvent(FDSAPI_SESSION_EVENT* sessionEvent)
{
	return 2 * FDSAPI_SizeOfUINT32();
}

static BOOL FDSAPI_DecodeSessionEvent(wStream* s, FDSAPI_SESSION_EVENT* sessionEvent)
{
	if (!FDSAPI_DecodeUINT32(s, &sessionEvent->sessionId)) return FALSE;
	if (!FDSAPI_DecodeUINT32(s, &sessionEvent->stateChange)) return FALSE;

	return TRUE;
}

static void FDSAPI_EncodeSessionEvent(wStream *s, FDSAPI_SESSION_EVENT* sessionEvent)
{
	FDSAPI_EncodeUINT32(s, sessionEvent->sessionId);
	FDSAPI_EncodeUINT32(s, sessionEvent->stateChange);
}

static void FDSAPI_FreeSessionEvent(FDSAPI_SESSION_EVENT* sessionEvent)
{
}


/* Message encoder/decoder */

static wStream* FDSAPI_AllocateStream(FDSAPI_MESSAGE* msg)
{
	UINT32 size;

	if (msg == NULL) return NULL;

	size = 2 * FDSAPI_SizeOfUINT16();

	switch (msg->messageId)
	{
		case FDSAPI_PING_REQUEST_ID:
			size += FDSAPI_SizeOfPingRequest(&msg->u.pingRequest);
			break;

		case FDSAPI_PING_RESPONSE_ID:
			size += FDSAPI_SizeOfPingResponse(&msg->u.pingResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST_ID:
			size += FDSAPI_SizeOfVirtualChannelOpenRequest(&msg->u.virtualChannelOpenRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE_ID:
			size += FDSAPI_SizeOfVirtualChannelOpenResponse(&msg->u.virtualChannelOpenResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST_ID:
			size += FDSAPI_SizeOfVirtualChannelOpenExRequest(&msg->u.virtualChannelOpenExRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE_ID:
			size += FDSAPI_SizeOfVirtualChannelOpenExResponse(&msg->u.virtualChannelOpenExResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST_ID:
			size += FDSAPI_SizeOfVirtualChannelCloseRequest(&msg->u.virtualChannelCloseRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE_ID:
			size += FDSAPI_SizeOfVirtualChannelCloseResponse(&msg->u.virtualChannelCloseResponse);
			break;

		case FDSAPI_DISCONNECT_SESSION_REQUEST_ID:
			size += FDSAPI_SizeOfDisconnectSessionRequest(&msg->u.disconnectSessionRequest);
			break;

		case FDSAPI_DISCONNECT_SESSION_RESPONSE_ID:
			size += FDSAPI_SizeOfDisconnectSessionResponse(&msg->u.disconnectSessionResponse);
			break;

		case FDSAPI_LOGOFF_SESSION_REQUEST_ID:
			size += FDSAPI_SizeOfLogoffSessionRequest(&msg->u.logoffSessionRequest);
			break;

		case FDSAPI_LOGOFF_SESSION_RESPONSE_ID:
			size += FDSAPI_SizeOfLogoffSessionResponse(&msg->u.logoffSessionResponse);
			break;

		case FDSAPI_SHUTDOWN_SYSTEM_REQUEST_ID:
			size += FDSAPI_SizeOfShutdownSystemRequest(&msg->u.shutdownSystemRequest);
			break;

		case FDSAPI_SHUTDOWN_SYSTEM_RESPONSE_ID:
			size += FDSAPI_SizeOfShutdownSystemResponse(&msg->u.shutdownSystemResponse);
			break;

		case FDSAPI_ENUMERATE_SESSIONS_REQUEST_ID:
			size += FDSAPI_SizeOfEnumerateSessionsRequest(&msg->u.enumerateSessionsRequest);
			break;

		case FDSAPI_ENUMERATE_SESSIONS_RESPONSE_ID:
			size += FDSAPI_SizeOfEnumerateSessionsResponse(&msg->u.enumerateSessionsResponse);
			break;

		case FDSAPI_QUERY_SESSION_INFORMATION_REQUEST_ID:
			size += FDSAPI_SizeOfQuerySessionInformationRequest(&msg->u.querySessionInformationRequest);
			break;

		case FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE_ID:
			size += FDSAPI_SizeOfQuerySessionInformationResponse(&msg->u.querySessionInformationResponse);
			break;

		case FDSAPI_AUTHENTICATE_USER_REQUEST_ID:
			size += FDSAPI_SizeOfAuthenticateUserRequest(&msg->u.authenticateUserRequest);
			break;

		case FDSAPI_AUTHENTICATE_USER_RESPONSE_ID:
			size += FDSAPI_SizeOfAuthenticateUserResponse(&msg->u.authenticateUserResponse);
			break;

		case FDSAPI_SESSION_EVENT_ID:
			size += FDSAPI_SizeOfSessionEvent(&msg->u.sessionEvent);
			break;

		default:
			return NULL;
	}

	return Stream_New(NULL, size);
}

BOOL FDSAPI_DecodeMessage(wStream* s, FDSAPI_MESSAGE* msg)
{
	BOOL result = FALSE;

	if (s == NULL) return FALSE;
	if (msg == NULL) return FALSE;

	ZeroMemory(msg, sizeof(FDSAPI_MESSAGE));

	if (!FDSAPI_DecodeUINT16(s, &msg->messageId)) return FALSE;
	if (!FDSAPI_DecodeUINT16(s, &msg->requestId)) return FALSE;

	switch (msg->messageId)
	{
		case FDSAPI_PING_REQUEST_ID:
			result = FDSAPI_DecodePingRequest(s, &msg->u.pingRequest);
			break;

		case FDSAPI_PING_RESPONSE_ID:
			result = FDSAPI_DecodePingResponse(s, &msg->u.pingResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST_ID:
			result = FDSAPI_DecodeVirtualChannelOpenRequest(s, &msg->u.virtualChannelOpenRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE_ID:
			result = FDSAPI_DecodeVirtualChannelOpenResponse(s, &msg->u.virtualChannelOpenResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST_ID:
			result = FDSAPI_DecodeVirtualChannelOpenExRequest(s, &msg->u.virtualChannelOpenExRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE_ID:
			result = FDSAPI_DecodeVirtualChannelOpenExResponse(s, &msg->u.virtualChannelOpenExResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST_ID:
			result = FDSAPI_DecodeVirtualChannelCloseRequest(s, &msg->u.virtualChannelCloseRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE_ID:
			result = FDSAPI_DecodeVirtualChannelCloseResponse(s, &msg->u.virtualChannelCloseResponse);
			break;

		case FDSAPI_DISCONNECT_SESSION_REQUEST_ID:
			result = FDSAPI_DecodeDisconnectSessionRequest(s, &msg->u.disconnectSessionRequest);
			break;

		case FDSAPI_DISCONNECT_SESSION_RESPONSE_ID:
			result = FDSAPI_DecodeDisconnectSessionResponse(s, &msg->u.disconnectSessionResponse);
			break;

		case FDSAPI_LOGOFF_SESSION_REQUEST_ID:
			result = FDSAPI_DecodeLogoffSessionRequest(s, &msg->u.logoffSessionRequest);
			break;

		case FDSAPI_LOGOFF_SESSION_RESPONSE_ID:
			result = FDSAPI_DecodeLogoffSessionResponse(s, &msg->u.logoffSessionResponse);
			break;

		case FDSAPI_SHUTDOWN_SYSTEM_REQUEST_ID:
			result = FDSAPI_DecodeShutdownSystemRequest(s, &msg->u.shutdownSystemRequest);
			break;

		case FDSAPI_SHUTDOWN_SYSTEM_RESPONSE_ID:
			result = FDSAPI_DecodeShutdownSystemResponse(s, &msg->u.shutdownSystemResponse);
			break;

		case FDSAPI_ENUMERATE_SESSIONS_REQUEST_ID:
			result = FDSAPI_DecodeEnumerateSessionsRequest(s, &msg->u.enumerateSessionsRequest);
			break;

		case FDSAPI_ENUMERATE_SESSIONS_RESPONSE_ID:
			result = FDSAPI_DecodeEnumerateSessionsResponse(s, &msg->u.enumerateSessionsResponse);
			break;

		case FDSAPI_QUERY_SESSION_INFORMATION_REQUEST_ID:
			result = FDSAPI_DecodeQuerySessionInformationRequest(s, &msg->u.querySessionInformationRequest);
			break;

		case FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE_ID:
			result = FDSAPI_DecodeQuerySessionInformationResponse(s, &msg->u.querySessionInformationResponse);
			break;

		case FDSAPI_AUTHENTICATE_USER_REQUEST_ID:
			result = FDSAPI_DecodeAuthenticateUserRequest(s, &msg->u.authenticateUserRequest);
			break;

		case FDSAPI_AUTHENTICATE_USER_RESPONSE_ID:
			result = FDSAPI_DecodeAuthenticateUserResponse(s, &msg->u.authenticateUserResponse);
			break;

		case FDSAPI_SESSION_EVENT_ID:
			result = FDSAPI_DecodeSessionEvent(s, &msg->u.sessionEvent);
			break;

		default:
			break;
	}

	return result;
}

wStream* FDSAPI_EncodeMessage(FDSAPI_MESSAGE* msg)
{
	wStream* s;

	/* Allocate a new stream. */
	s = FDSAPI_AllocateStream(msg);

	if (!s)
		return s;

	/* Encode each of the messages. */
	FDSAPI_EncodeUINT16(s, msg->messageId);
	FDSAPI_EncodeUINT16(s, msg->requestId);

	switch (msg->messageId)
	{
		case FDSAPI_PING_REQUEST_ID:
			FDSAPI_EncodePingRequest(s, &msg->u.pingRequest);
			break;

		case FDSAPI_PING_RESPONSE_ID:
			FDSAPI_EncodePingResponse(s, &msg->u.pingResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST_ID:
			FDSAPI_EncodeVirtualChannelOpenRequest(s, &msg->u.virtualChannelOpenRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE_ID:
			FDSAPI_EncodeVirtualChannelOpenResponse(s, &msg->u.virtualChannelOpenResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST_ID:
			FDSAPI_EncodeVirtualChannelOpenExRequest(s, &msg->u.virtualChannelOpenExRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE_ID:
			FDSAPI_EncodeVirtualChannelOpenExResponse(s, &msg->u.virtualChannelOpenExResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST_ID:
			FDSAPI_EncodeVirtualChannelCloseRequest(s, &msg->u.virtualChannelCloseRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE_ID:
			FDSAPI_EncodeVirtualChannelCloseResponse(s, &msg->u.virtualChannelCloseResponse);
			break;

		case FDSAPI_DISCONNECT_SESSION_REQUEST_ID:
			FDSAPI_EncodeDisconnectSessionRequest(s, &msg->u.disconnectSessionRequest);
			break;

		case FDSAPI_DISCONNECT_SESSION_RESPONSE_ID:
			FDSAPI_EncodeDisconnectSessionResponse(s, &msg->u.disconnectSessionResponse);
			break;

		case FDSAPI_LOGOFF_SESSION_REQUEST_ID:
			FDSAPI_EncodeLogoffSessionRequest(s, &msg->u.logoffSessionRequest);
			break;

		case FDSAPI_LOGOFF_SESSION_RESPONSE_ID:
			FDSAPI_EncodeLogoffSessionResponse(s, &msg->u.logoffSessionResponse);
			break;

		case FDSAPI_SHUTDOWN_SYSTEM_REQUEST_ID:
			FDSAPI_EncodeShutdownSystemRequest(s, &msg->u.shutdownSystemRequest);
			break;

		case FDSAPI_SHUTDOWN_SYSTEM_RESPONSE_ID:
			FDSAPI_EncodeShutdownSystemResponse(s, &msg->u.shutdownSystemResponse);
			break;

		case FDSAPI_ENUMERATE_SESSIONS_REQUEST_ID:
			FDSAPI_EncodeEnumerateSessionsRequest(s, &msg->u.enumerateSessionsRequest);
			break;

		case FDSAPI_ENUMERATE_SESSIONS_RESPONSE_ID:
			FDSAPI_EncodeEnumerateSessionsResponse(s, &msg->u.enumerateSessionsResponse);
			break;

		case FDSAPI_QUERY_SESSION_INFORMATION_REQUEST_ID:
			FDSAPI_EncodeQuerySessionInformationRequest(s, &msg->u.querySessionInformationRequest);
			break;

		case FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE_ID:
			FDSAPI_EncodeQuerySessionInformationResponse(s, &msg->u.querySessionInformationResponse);
			break;

		case FDSAPI_AUTHENTICATE_USER_REQUEST_ID:
			FDSAPI_EncodeAuthenticateUserRequest(s, &msg->u.authenticateUserRequest);
			break;

		case FDSAPI_AUTHENTICATE_USER_RESPONSE_ID:
			FDSAPI_EncodeAuthenticateUserResponse(s, &msg->u.authenticateUserResponse);
			break;

		case FDSAPI_SESSION_EVENT_ID:
			FDSAPI_EncodeSessionEvent(s, &msg->u.sessionEvent);
			break;

		default:
			/* Free the stream. */
			Stream_Free(s, TRUE);
			s = NULL;

			break;
	}

	if (s)
	{
		Stream_SealLength(s);
	}

	return s;
}

void FDSAPI_FreeMessage(FDSAPI_MESSAGE* msg)
{
	switch (msg->messageId)
	{
		case FDSAPI_PING_REQUEST_ID:
			FDSAPI_FreePingRequest(&msg->u.pingRequest);
			break;

		case FDSAPI_PING_RESPONSE_ID:
			FDSAPI_FreePingResponse(&msg->u.pingResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_REQUEST_ID:
			FDSAPI_FreeVirtualChannelOpenRequest(&msg->u.virtualChannelOpenRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_RESPONSE_ID:
			FDSAPI_FreeVirtualChannelOpenResponse(&msg->u.virtualChannelOpenResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_REQUEST_ID:
			FDSAPI_FreeVirtualChannelOpenExRequest(&msg->u.virtualChannelOpenExRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_OPEN_EX_RESPONSE_ID:
			FDSAPI_FreeVirtualChannelOpenExResponse(&msg->u.virtualChannelOpenExResponse);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_CLOSE_REQUEST_ID:
			FDSAPI_FreeVirtualChannelCloseRequest(&msg->u.virtualChannelCloseRequest);
			break;

		case FDSAPI_VIRTUAL_CHANNEL_CLOSE_RESPONSE_ID:
			FDSAPI_FreeVirtualChannelCloseResponse(&msg->u.virtualChannelCloseResponse);
			break;

		case FDSAPI_DISCONNECT_SESSION_REQUEST_ID:
			FDSAPI_FreeDisconnectSessionRequest(&msg->u.disconnectSessionRequest);
			break;

		case FDSAPI_DISCONNECT_SESSION_RESPONSE_ID:
			FDSAPI_FreeDisconnectSessionResponse(&msg->u.disconnectSessionResponse);
			break;

		case FDSAPI_LOGOFF_SESSION_REQUEST_ID:
			FDSAPI_FreeLogoffSessionRequest(&msg->u.logoffSessionRequest);
			break;

		case FDSAPI_LOGOFF_SESSION_RESPONSE_ID:
			FDSAPI_FreeLogoffSessionResponse(&msg->u.logoffSessionResponse);
			break;

		case FDSAPI_SHUTDOWN_SYSTEM_REQUEST_ID:
			FDSAPI_FreeShutdownSystemRequest(&msg->u.shutdownSystemRequest);
			break;

		case FDSAPI_SHUTDOWN_SYSTEM_RESPONSE_ID:
			FDSAPI_FreeShutdownSystemResponse(&msg->u.shutdownSystemResponse);
			break;

		case FDSAPI_ENUMERATE_SESSIONS_REQUEST_ID:
			FDSAPI_FreeEnumerateSessionsRequest(&msg->u.enumerateSessionsRequest);
			break;

		case FDSAPI_ENUMERATE_SESSIONS_RESPONSE_ID:
			FDSAPI_FreeEnumerateSessionsResponse(&msg->u.enumerateSessionsResponse);
			break;

		case FDSAPI_QUERY_SESSION_INFORMATION_REQUEST_ID:
			FDSAPI_FreeQuerySessionInformationRequest(&msg->u.querySessionInformationRequest);
			break;

		case FDSAPI_QUERY_SESSION_INFORMATION_RESPONSE_ID:
			FDSAPI_FreeQuerySessionInformationResponse(&msg->u.querySessionInformationResponse);
			break;

		case FDSAPI_AUTHENTICATE_USER_REQUEST_ID:
			FDSAPI_FreeAuthenticateUserRequest(&msg->u.authenticateUserRequest);
			break;

		case FDSAPI_AUTHENTICATE_USER_RESPONSE_ID:
			FDSAPI_FreeAuthenticateUserResponse(&msg->u.authenticateUserResponse);
			break;

		case FDSAPI_SESSION_EVENT_ID:
			FDSAPI_FreeSessionEvent(&msg->u.sessionEvent);
			break;

		default:
			break;
	}
}
