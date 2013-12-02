 /**
 * Rpc engine build upon google protocol buffers
 *
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thinstuff.at>
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

#include <winpr/pipe.h>
#include <winpr/thread.h>
#include <call/CallFactory.h>
#include <arpa/inet.h>
#include <winpr/wlog.h>
#include <call/CallIn.h>
#include <appcontext/ApplicationContext.h>

namespace freerds {
namespace pbrpc {

#define CLIENT_DISCONNECTED 2
#define CLIENT_ERROR -1
#define CLIENT_SUCCESS 0

static wLog* logger_RPCEngine = WLog_Get("freerds.pbrpc.RpcEngine");

RpcEngine::RpcEngine() :
		mPacktLength(0), mHeaderRead(0), mPayloadRead(0)
{
	mhStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	WLog_SetLogLevel(logger_RPCEngine, WLOG_ERROR);
}

RpcEngine::~RpcEngine()
{
	google::protobuf::ShutdownProtobufLibrary();
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
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "creating namedpipe failed");
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
	mhServerPipe = createServerPipe("\\\\.\\pipe\\FreeRDS_SessionManager");

	if (!mhServerPipe)
	{
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "Could not create named pipe \\\\.\\pipe\\FreeRDS_SessionManager");
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
			4 - mHeaderRead, &lpNumberOfBytesRead, NULL);

	if (!fSuccess || (lpNumberOfBytesRead == 0))
	{
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "error reading");
		return CLIENT_DISCONNECTED;
	}

	mHeaderRead += lpNumberOfBytesRead;

	if (mHeaderRead == 4)
	{
		mPacktLength = ntohl(*(DWORD *)mHeaderBuffer);
		WLog_Print(logger_RPCEngine, WLOG_TRACE, "header read, packet size %d",mPacktLength);
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
	mpbRPC.Clear();
	mpbRPC.ParseFromArray(mPayloadBuffer, mPayloadRead);

	uint32_t callID = mpbRPC.tag();
	uint32_t callType = mpbRPC.msgtype();

	if (mpbRPC.isresponse())
	{
		// search the stored call to fill back answer
		callNS::CallOut* foundCallOut = 0;
		std::list<callNS::CallOut*>::iterator it;

		for ( it = mAnswerWaitingQueue.begin(); it != mAnswerWaitingQueue.end(); it++)
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
			// fill the answer and signal
			if (mpbRPC.status() == RPCBase_RPCSTATUS_SUCCESS)
			{
				foundCallOut->setEncodedeResponse(mpbRPC.payload());
				foundCallOut->decodeResponse();
				foundCallOut->setResult(0);
			}
			else if (mpbRPC.status() == RPCBase_RPCSTATUS_FAILED)
			{
				foundCallOut->setErrorDescription(mpbRPC.errordescription());
				foundCallOut->setResult(1);
			}
			else if (mpbRPC.status() == RPCBase_RPCSTATUS_NOTFOUND)
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
			// call not found ... send error
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "no registered class for calltype=%d",callType);
			sendError(callID, callType);
			delete createdCall;
			return CLIENT_ERROR;
		}

		if (createdCall->getDerivedType() == 1)
		{
			// we got an CallIn object ... so handle it
			callNS::CallIn* createdCallIn = (callNS::CallIn*) createdCall;
			createdCallIn->setEncodedRequest(mpbRPC.payload());
			createdCallIn->setTag(callID);

			WLog_Print(logger_RPCEngine, WLOG_TRACE, "call upacked for callType=%d and callID=%d",callType,callID);

			// call the implementation ...
			createdCallIn->decodeRequest();

			createdCallIn->doStuff();

			createdCallIn->encodeResponse();
			// send the result
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

int RpcEngine::send(freerds::sessionmanager::call::Call* call)
{
	DWORD lpNumberOfBytesWritten;
	std::string serialized;

	if (call->getDerivedType() == 1)
	{
		// this is a CallIn
		callNS::CallIn* callIn = (callNS::CallIn*) call;
		// create answer
		mpbRPC.Clear();
		mpbRPC.set_isresponse(true);
		mpbRPC.set_tag(callIn->getTag());
		mpbRPC.set_msgtype(callIn->getCallType());

		if (call->getResult())
		{
			WLog_Print(logger_RPCEngine, WLOG_TRACE, "call for callType=%d and callID=%d failed, sending error response",callIn->getCallType(),callIn->getTag());
			mpbRPC.set_status(RPCBase_RPCSTATUS_FAILED);
			std::string errordescription = callIn->getErrorDescription();

			if (errordescription.size() > 0)
			{
				mpbRPC.set_errordescription(errordescription);
			}
		}
		else
		{
			WLog_Print(logger_RPCEngine, WLOG_TRACE, "call for callType=%d and callID=%d success, sending response",callIn->getCallType(),callIn->getTag());
			mpbRPC.set_status(RPCBase_RPCSTATUS_SUCCESS);
			mpbRPC.set_payload(callIn->getEncodedResponse());
		}
	}
	else if (call->getDerivedType() == 2)
	{
		// this is a CallOut
		callNS::CallOut* callOut = (callNS::CallOut*) call;
		// create answer
		mpbRPC.Clear();
		mpbRPC.set_isresponse(false);
		mpbRPC.set_tag(callOut->getTag());
		mpbRPC.set_msgtype(callOut->getCallType());
		mpbRPC.set_status(RPCBase_RPCSTATUS_SUCCESS);
		mpbRPC.set_payload(callOut->getEncodedRequest());
	}

	mpbRPC.SerializeToString(&serialized);
	return sendInternal(serialized);
}

int RpcEngine::sendError(uint32_t callID, uint32_t callType)
{
	std::string serialized;

	mpbRPC.Clear();
	mpbRPC.set_isresponse(true);
	mpbRPC.set_tag(callID);
	mpbRPC.set_msgtype(callType);
	mpbRPC.set_status(RPCBase_RPCSTATUS_NOTFOUND);

	mpbRPC.SerializeToString(&serialized);

	return sendInternal(serialized);
}

int RpcEngine::sendInternal(std::string data)
{
	BOOL fSuccess;
	DWORD lpNumberOfBytesWritten;
	DWORD messageSize = htonl(data.size());

	fSuccess = WriteFile(mhClientPipe, &messageSize,
			4, &lpNumberOfBytesWritten, NULL);

	if (!fSuccess || (lpNumberOfBytesWritten == 0))
	{
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending");
		return CLIENT_ERROR;
	}

	fSuccess = WriteFile(mhClientPipe, data.c_str(),
			data.size(), &lpNumberOfBytesWritten, NULL);

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
	SignalingQueue<callNS::Call *>* outgoingQueue = APP_CONTEXT.getRpcOutgoingQueue();
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

			// process the data
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

int RpcEngine::processOutgoingCall(freerds::sessionmanager::call::Call* call)
{
	if (call->getDerivedType() == 2)
	{
			// this is a CallOut
			callNS::CallOut* callOut = (callNS::CallOut*) call;
			callOut->encodeRequest();

			if (send(call) == CLIENT_SUCCESS)
			{
				mAnswerWaitingQueue.push_back(callOut);
				return CLIENT_SUCCESS;
			}
			else
			{
				WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending call, informing call");
				callOut->setResult(1); // for failed
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
