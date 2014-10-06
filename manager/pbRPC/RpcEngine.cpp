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

#include <appcontext/ApplicationContext.h>

namespace freerds
{
	namespace pbrpc
	{
		#define CLIENT_DISCONNECTED	2
		#define CLIENT_ERROR		-1
		#define CLIENT_SUCCESS		0

		static wLog* logger_RPCEngine = WLog_Get("freerds.pbrpc.RpcEngine");

		RpcEngine::RpcEngine() :
				mPacktLength(0), mHeaderRead(0), mPayloadRead(0), mNextOutCall(1),
				mhClientPipe(0), mhServerPipe(0), mhServerThread(0)
		{
			mHeaderBuffer = (BYTE*) &m_Header;
			mhStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
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
			mhServerThread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE) RpcEngine::listenerThread, (void*) this,
					CREATE_SUSPENDED, NULL);

			ResumeThread(mhServerThread);

			return CLIENT_SUCCESS;
		}

		int RpcEngine::stopEngine()
		{
			if (mhServerThread)
			{
				SetEvent(mhStopEvent);
				WaitForSingleObject(mhServerThread, INFINITE);
				CloseHandle(mhServerThread);
				mhServerThread = NULL;
			}

			return CLIENT_SUCCESS;
		}

		int RpcEngine::createServerPipe(void)
		{
			mhServerPipe = createServerPipe("\\\\.\\pipe\\FreeRDS_Manager");

			if (!mhServerPipe)
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

			if (!mhServerPipe)
				return NULL;

			nCount = 0;
			events[nCount++] = mhStopEvent;
			events[nCount++] = mhServerPipe;

			status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

			if (WaitForSingleObject(mhStopEvent, 0) == WAIT_OBJECT_0)
			{
				WLog_Print(logger_RPCEngine, WLOG_TRACE, "got shutdown signal");
				return NULL;
			}

			if (WaitForSingleObject(mhServerPipe, 0) == WAIT_OBJECT_0)
			{
				BOOL fConnected;
				DWORD dwPipeMode;

				fConnected = ConnectNamedPipe(mhServerPipe, NULL);

				if (!fConnected)
					fConnected = (GetLastError() == ERROR_PIPE_CONNECTED);

				if (!fConnected)
				{
					WLog_Print(logger_RPCEngine, WLOG_ERROR, "could not connect client");
					return NULL;
				}

				mhClientPipe = mhServerPipe;

				dwPipeMode = PIPE_WAIT;
				SetNamedPipeHandleState(mhClientPipe, &dwPipeMode, NULL, NULL);
				WLog_Print(logger_RPCEngine, WLOG_TRACE, "connect client with handle %x",mhClientPipe);

				return mhClientPipe;
			}

			return NULL;
		}

		int RpcEngine::read()
		{
			if (mPacktLength > 0)
				return readPayload();
			else
				return readHeader();
		}

		int RpcEngine::readHeader()
		{
			BOOL fSuccess;
			DWORD lpNumberOfBytesRead = 0;

			fSuccess = ReadFile(mhClientPipe, mHeaderBuffer + mHeaderRead,
					FDSAPI_MSG_HEADER_SIZE - mHeaderRead, &lpNumberOfBytesRead, NULL);

			if (!fSuccess || (lpNumberOfBytesRead == 0))
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "error reading");
				return CLIENT_DISCONNECTED;
			}

			mHeaderRead += lpNumberOfBytesRead;

			if (mHeaderRead == FDSAPI_MSG_HEADER_SIZE)
			{
				FDSAPI_MSG_HEADER* header = (FDSAPI_MSG_HEADER*) mHeaderBuffer;
				mPacktLength = header->msgSize;
				WLog_Print(logger_RPCEngine, WLOG_TRACE, "header read, packet size %d", mPacktLength);
			}

			return CLIENT_SUCCESS;
		}

		int RpcEngine::readPayload()
		{
			BOOL fSuccess;
			DWORD lpNumberOfBytesRead = 0;

			fSuccess = ReadFile(mhClientPipe, mPayloadBuffer + mPayloadRead,
					mPacktLength - mPayloadRead, &lpNumberOfBytesRead, NULL);

			if (!fSuccess || (lpNumberOfBytesRead == 0))
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "error reading payload");
				return CLIENT_DISCONNECTED;
			}

			mPayloadRead += lpNumberOfBytesRead;

			return CLIENT_SUCCESS;
		}

		int RpcEngine::processData()
		{
			UINT32 callID;
			UINT32 callType;
			std::string payload;

			callID = m_Header.callId;
			callType = m_Header.msgType;

			payload.assign((const char*) mPayloadBuffer, (size_t) mPayloadRead);

			if (FDSAPI_IS_RESPONSE_ID(callType))
			{
				callNS::CallOut* foundCallOut = 0;
				std::list<callNS::CallOut*>::iterator it;

				for (it = mAnswerWaitingQueue.begin(); it != mAnswerWaitingQueue.end(); it++)
				{
					callNS::CallOut* currentCallOut = (callNS::CallOut*)(*it);

					if (currentCallOut->getTag() == callID)
					{
						foundCallOut = currentCallOut;
						mAnswerWaitingQueue.remove(foundCallOut);
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
				callNS::Call* createdCall = CALL_FACTORY.createClass(callType);

				if (!createdCall)
				{
					WLog_Print(logger_RPCEngine, WLOG_ERROR, "no registered class for calltype=%d",callType);
					sendError(callID, callType);
					delete createdCall;
					return CLIENT_ERROR;
				}

				if (createdCall->getDerivedType() == 1)
				{
					callNS::CallIn* createdCallIn = (callNS::CallIn*) createdCall;

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

		int RpcEngine::send(freerds::call::Call* call)
		{
			std::string serialized;
			FDSAPI_MSG_HEADER header;
			DWORD lpNumberOfBytesWritten;

			if (call->getDerivedType() == 1)
			{
				callNS::CallIn* callIn = (callNS::CallIn*) call;

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
				callNS::CallOut* callOut = (callNS::CallOut*) call;

				header.msgType = FDSAPI_REQUEST_ID(callOut->getCallType());
				header.callId = callOut->getTag();
				header.status = FDSAPI_STATUS_SUCCESS;
				serialized = callOut->getEncodedRequest();
			}

			header.msgSize = (UINT32) serialized.size();

			return sendInternal(&header, (BYTE*) serialized.data());
		}

		int RpcEngine::sendError(uint32_t callID, uint32_t callType)
		{
			std::string serialized;
			FDSAPI_MSG_HEADER header;

			header.msgType = FDSAPI_RESPONSE_ID(callType);
			header.callId = callID;
			header.status = FDSAPI_STATUS_NOTFOUND;
			header.msgSize = 0;

			return sendInternal(&header, NULL);
		}

		int RpcEngine::sendInternal(FDSAPI_MSG_HEADER* header, BYTE* buffer)
		{
			BOOL fSuccess;
			DWORD lpNumberOfBytesWritten;

			fSuccess = WriteFile(mhClientPipe, header,
					FDSAPI_MSG_HEADER_SIZE, &lpNumberOfBytesWritten, NULL);

			if (!fSuccess || (lpNumberOfBytesWritten == 0))
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending");
				return CLIENT_ERROR;
			}

			fSuccess = WriteFile(mhClientPipe, buffer,
					header->msgSize, &lpNumberOfBytesWritten, NULL);

			if (!fSuccess || (lpNumberOfBytesWritten == 0))
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending");
				return CLIENT_ERROR;
			}

			return CLIENT_SUCCESS;
		}

		void RpcEngine::resetStatus()
		{
			mPacktLength = 0;
			mHeaderRead = 0;
			mPayloadRead = 0;
		}

		int RpcEngine::serveClient()
		{
			DWORD status;
			DWORD nCount;
			SignalingQueue<callNS::Call*>* outgoingQueue = APP_CONTEXT.getRpcOutgoingQueue();
			HANDLE queueHandle = outgoingQueue->getSignalHandle();
			HANDLE events[3];

			nCount = 0;
			events[nCount++] = mhStopEvent;
			events[nCount++] = mhClientPipe;
			events[nCount++] = queueHandle;

			int retValue = 0;

			while (1)
			{
				status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

				if (WaitForSingleObject(mhStopEvent, 0) == WAIT_OBJECT_0)
				{
					retValue = CLIENT_ERROR;
					break;
				}

				if (WaitForSingleObject(mhClientPipe, 0) == WAIT_OBJECT_0)
				{
					retValue = read();

					if (retValue)
						break;

					if (mPayloadRead == mPacktLength)
					{
						processData();
						resetStatus();
					}
				}

				if (WaitForSingleObject(queueHandle, 0) == WAIT_OBJECT_0)
				{
					outgoingQueue->resetEventAndLockQueue();
					callNS::Call* currentCall = outgoingQueue->getElementLockFree();

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

		int RpcEngine::processOutgoingCall(freerds::call::Call* call)
		{
			if (call->getDerivedType() == 2)
			{
				callNS::CallOut* callOut = (callNS::CallOut*) call;
				callOut->encodeRequest();
				callOut->setTag(mNextOutCall++);

				if (send(call) == CLIENT_SUCCESS)
				{
					mAnswerWaitingQueue.push_back(callOut);
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
}
