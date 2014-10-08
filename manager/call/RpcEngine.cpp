 /**
 * Rpc engine build upon google protocol buffers
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

#include "RpcEngine.h"

#include <winpr/wlog.h>
#include <winpr/pipe.h>
#include <winpr/thread.h>

#include <call/CallIn.h>
#include <call/CallFactory.h>

#include <session/ApplicationContext.h>

#define CLIENT_DISCONNECTED	2
#define CLIENT_ERROR		-1
#define CLIENT_SUCCESS		0

namespace freerds
{
	static wLog* logger_RPCEngine = WLog_Get("freerds.RpcEngine");

	RpcEngine::RpcEngine()
	: m_PacketLength(0), m_HeaderRead(0), m_PayloadRead(0), m_NextOutCall(1),
	  m_hClientPipe(0), m_hServerPipe(0), m_hServerThread(0)
	{
		m_HeaderBuffer = (BYTE*) &m_Header;
		m_hStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
		WLog_SetLogLevel(logger_RPCEngine, WLOG_ERROR);
	}

	RpcEngine::~RpcEngine()
	{

	}

	HANDLE RpcEngine::createServerPipe(const char* endpoint)
	{
		DWORD dwPipeMode;
		HANDLE hNamedPipe;

		dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

		hNamedPipe = CreateNamedPipe(endpoint, PIPE_ACCESS_DUPLEX,
				dwPipeMode, PIPE_UNLIMITED_INSTANCES,
				PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, 0, NULL);

		if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE))
		{
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "creating named pipe failed");
			return NULL;
		}

		return hNamedPipe;
	}

	int RpcEngine::startEngine()
	{
		m_hServerThread = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE) RpcEngine::listenerThread, (void*) this,
				CREATE_SUSPENDED, NULL);

		ResumeThread(m_hServerThread);

		return CLIENT_SUCCESS;
	}

	int RpcEngine::stopEngine()
	{
		if (m_hServerThread)
		{
			SetEvent(m_hStopEvent);
			WaitForSingleObject(m_hServerThread, INFINITE);
			CloseHandle(m_hServerThread);
			m_hServerThread = NULL;
		}

		return CLIENT_SUCCESS;
	}

	int RpcEngine::createServerPipe(void)
	{
		m_hServerPipe = createServerPipe("\\\\.\\pipe\\FreeRDS_Manager");

		if (!m_hServerPipe)
		{
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "Could not create named pipe \\\\.\\pipe\\FreeRDS_Manager");
			return CLIENT_ERROR;
		}

		return CLIENT_SUCCESS;
	}

	void* RpcEngine::listenerThread(void* arg)
	{
		RpcEngine* engine;

		engine = (RpcEngine*) arg;

		WLog_Print(logger_RPCEngine, WLOG_TRACE, "started RPC listener Thread");

		while (1)
		{
			if (engine->createServerPipe() != CLIENT_SUCCESS)
			{
				break;
			}

			HANDLE clientPipe = engine->acceptClient();

			if (!clientPipe)
				break;

			if (engine->serveClient() == CLIENT_ERROR)
			{
				break;
			}

			engine->resetStatus();
			APP_CONTEXT.rpcDisconnected();
		}

		return NULL;
	}

	HANDLE RpcEngine::acceptClient()
	{
		DWORD status;
		DWORD nCount;
		HANDLE events[2];

		if (!m_hServerPipe)
			return NULL;

		nCount = 0;
		events[nCount++] = m_hStopEvent;
		events[nCount++] = m_hServerPipe;

		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(m_hStopEvent, 0) == WAIT_OBJECT_0)
		{
			WLog_Print(logger_RPCEngine, WLOG_TRACE, "got shutdown signal");
			return NULL;
		}

		if (WaitForSingleObject(m_hServerPipe, 0) == WAIT_OBJECT_0)
		{
			BOOL fConnected;
			DWORD dwPipeMode;

			fConnected = ConnectNamedPipe(m_hServerPipe, NULL);

			if (!fConnected)
				fConnected = (GetLastError() == ERROR_PIPE_CONNECTED);

			if (!fConnected)
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "could not connect client");
				return NULL;
			}

			m_hClientPipe = m_hServerPipe;

			dwPipeMode = PIPE_WAIT;
			SetNamedPipeHandleState(m_hClientPipe, &dwPipeMode, NULL, NULL);
			WLog_Print(logger_RPCEngine, WLOG_TRACE, "connect client with handle %x", m_hClientPipe);

			return m_hClientPipe;
		}

		return NULL;
	}

	int RpcEngine::read()
	{
		if (m_PacketLength > 0)
			return readPayload();
		else
			return readHeader();
	}

	int RpcEngine::readHeader()
	{
		BOOL fSuccess;
		DWORD lpNumberOfBytesRead = 0;

		fSuccess = ReadFile(m_hClientPipe, m_HeaderBuffer + m_HeaderRead,
				FDSAPI_MSG_HEADER_SIZE - m_HeaderRead, &lpNumberOfBytesRead, NULL);

		if (!fSuccess || (lpNumberOfBytesRead == 0))
		{
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "error reading");
			return CLIENT_DISCONNECTED;
		}

		m_HeaderRead += lpNumberOfBytesRead;

		if (m_HeaderRead == FDSAPI_MSG_HEADER_SIZE)
		{
			FDSAPI_MSG_HEADER* header = (FDSAPI_MSG_HEADER*) m_HeaderBuffer;
			m_PacketLength = header->msgSize;
			WLog_Print(logger_RPCEngine, WLOG_TRACE, "header read, packet size %d", m_PacketLength);
		}

		return CLIENT_SUCCESS;
	}

	int RpcEngine::readPayload()
	{
		BOOL fSuccess;
		DWORD lpNumberOfBytesRead = 0;

		fSuccess = ReadFile(m_hClientPipe, m_PayloadBuffer + m_PayloadRead,
				m_PacketLength - m_PayloadRead, &lpNumberOfBytesRead, NULL);

		if (!fSuccess || (lpNumberOfBytesRead == 0))
		{
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "error reading payload");
			return CLIENT_DISCONNECTED;
		}

		m_PayloadRead += lpNumberOfBytesRead;

		return CLIENT_SUCCESS;
	}

	int RpcEngine::processData()
	{
		UINT32 callID;
		UINT32 callType;
		std::string payload;

		callID = m_Header.callId;
		callType = m_Header.msgType;

		payload.assign((const char*) m_PayloadBuffer, (size_t) m_PayloadRead);

		if (FDSAPI_IS_RESPONSE_ID(callType))
		{
			CallOut* foundCallOut = 0;
			std::list<CallOut*>::iterator it;

			for (it = m_AnswerWaitingQueue.begin(); it != m_AnswerWaitingQueue.end(); it++)
			{
				CallOut* currentCallOut = (CallOut*)(*it);

				if (currentCallOut->getTag() == callID)
				{
					foundCallOut = currentCallOut;
					m_AnswerWaitingQueue.remove(foundCallOut);
					break;
				}
			}

			if (!foundCallOut)
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "Received answer for callID %d, but no responding call was found",callID);
				return CLIENT_SUCCESS;
			}
			else
			{
				if (m_Header.status == FDSAPI_STATUS_SUCCESS)
				{
					foundCallOut->setEncodedeResponse(payload);
					foundCallOut->decodeResponse();
					foundCallOut->setResult(0);
				}
				else if (m_Header.status == FDSAPI_STATUS_FAILED)
				{
					foundCallOut->setResult(1);
				}
				else if (m_Header.status == FDSAPI_STATUS_NOTFOUND)
				{
					foundCallOut->setResult(2);
				}
			}
		}
		else
		{
			Call* createdCall = CALL_FACTORY.createClass(callType);

			if (!createdCall)
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "no registered class for calltype=%d",callType);
				sendError(callID, callType);
				delete createdCall;
				return CLIENT_ERROR;
			}

			if (createdCall->getDerivedType() == 1)
			{
				CallIn* createdCallIn = (CallIn*) createdCall;

				createdCallIn->setEncodedRequest(payload);
				createdCallIn->setTag(callID);

				WLog_Print(logger_RPCEngine, WLOG_TRACE, "call upacked for callType=%d and callID=%d",callType,callID);

				createdCallIn->decodeRequest();
				createdCallIn->doStuff();
				createdCallIn->encodeResponse();

				send(createdCall);
				delete createdCall;
			}
			else
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "callobject had wrong baseclass, callType=%d",callType);
				sendError(callID, callType);
				delete createdCall;
				return CLIENT_ERROR;
			}

			return CLIENT_SUCCESS;
		}

		return CLIENT_SUCCESS;
	}

	int RpcEngine::send(Call* call)
	{
		std::string serialized;
		FDSAPI_MSG_HEADER header;
		DWORD lpNumberOfBytesWritten;

		if (call->getDerivedType() == 1)
		{
			CallIn* callIn = (CallIn*) call;

			header.msgType = FDSAPI_RESPONSE_ID(callIn->getCallType());
			header.callId = callIn->getTag();
			header.status = FDSAPI_STATUS_SUCCESS;

			if (call->getResult())
			{
				header.status = FDSAPI_STATUS_FAILED;
			}
			else
			{
				header.status = FDSAPI_STATUS_SUCCESS;
				serialized = callIn->getEncodedResponse();
			}
		}
		else if (call->getDerivedType() == 2)
		{
			CallOut* callOut = (CallOut*) call;

			header.msgType = FDSAPI_REQUEST_ID(callOut->getCallType());
			header.callId = callOut->getTag();
			header.status = FDSAPI_STATUS_SUCCESS;
			serialized = callOut->getEncodedRequest();
		}

		header.msgSize = (UINT32) serialized.size();

		return sendInternal(&header, (BYTE*) serialized.data());
	}

	int RpcEngine::sendError(UINT32 callId, UINT32 msgType)
	{
		std::string serialized;
		FDSAPI_MSG_HEADER header;

		header.msgType = FDSAPI_RESPONSE_ID(msgType);
		header.callId = callId;
		header.status = FDSAPI_STATUS_NOTFOUND;
		header.msgSize = 0;

		return sendInternal(&header, NULL);
	}

	int RpcEngine::sendInternal(FDSAPI_MSG_HEADER* header, BYTE* buffer)
	{
		BOOL fSuccess;
		DWORD lpNumberOfBytesWritten;

		fSuccess = WriteFile(m_hClientPipe, header,
				FDSAPI_MSG_HEADER_SIZE, &lpNumberOfBytesWritten, NULL);

		if (!fSuccess || (lpNumberOfBytesWritten != FDSAPI_MSG_HEADER_SIZE))
		{
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending");
			return CLIENT_ERROR;
		}

		fSuccess = WriteFile(m_hClientPipe, buffer,
				header->msgSize, &lpNumberOfBytesWritten, NULL);

		if (!fSuccess || (lpNumberOfBytesWritten != header->msgSize))
		{
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending");
			return CLIENT_ERROR;
		}

		return CLIENT_SUCCESS;
	}

	void RpcEngine::resetStatus()
	{
		m_PacketLength = 0;
		m_HeaderRead = 0;
		m_PayloadRead = 0;
	}

	int RpcEngine::serveClient()
	{
		DWORD status;
		DWORD nCount;
		SignalingQueue<Call*>* outgoingQueue = APP_CONTEXT.getRpcOutgoingQueue();
		HANDLE queueHandle = outgoingQueue->getSignalHandle();
		HANDLE events[3];

		nCount = 0;
		events[nCount++] = m_hStopEvent;
		events[nCount++] = m_hClientPipe;
		events[nCount++] = queueHandle;

		int retValue = 0;

		while (1)
		{
			status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

			if (WaitForSingleObject(m_hStopEvent, 0) == WAIT_OBJECT_0)
			{
				retValue = CLIENT_ERROR;
				break;
			}

			if (WaitForSingleObject(m_hClientPipe, 0) == WAIT_OBJECT_0)
			{
				retValue = read();

				if (retValue)
					break;

				if (m_PayloadRead == m_PacketLength)
				{
					processData();
					resetStatus();
				}
			}

			if (WaitForSingleObject(queueHandle, 0) == WAIT_OBJECT_0)
			{
				outgoingQueue->resetEventAndLockQueue();
				Call* currentCall = outgoingQueue->getElementLockFree();

				while (currentCall)
				{
					processOutgoingCall(currentCall);
					currentCall = outgoingQueue->getElementLockFree();
				}

				outgoingQueue->unlockQueue();
			}
		}

		return retValue;
	}

	int RpcEngine::processOutgoingCall(Call* call)
	{
		if (call->getDerivedType() == 2)
		{
			CallOut* callOut = (CallOut*) call;

			callOut->encodeRequest();
			callOut->setTag(m_NextOutCall++);

			if (send(call) == CLIENT_SUCCESS)
			{
				m_AnswerWaitingQueue.push_back(callOut);
				return CLIENT_SUCCESS;
			}
			else
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending call, informing call");
				callOut->setResult(1);
				return CLIENT_ERROR;
			}
		}
		else
		{
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "call was no outgoing call, wrong type in queue, dropping packet!");
			delete call;
			return CLIENT_SUCCESS;
		}

		return 0;
	}
}
