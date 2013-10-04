#include "RpcEngine.h"

#include <winpr/pipe.h>
#include <winpr/thread.h>
#include <call/CallFactory.h>
#include <arpa/inet.h>
#include <winpr/wlog.h>
#include <call/CallIn.h>



namespace freerds {
namespace pbrpc {

#define CLIENT_DISCONNECTED 2
#define CLIENT_ERROR -1
#define CLIENT_SUCCESS 0

static wLog * logger_RPCEngine = WLog_Get("freerds.pbrpc.RpcEngine");


RpcEngine::RpcEngine() :
		mPacktLength(0), mHeaderRead(0), mPayloadRead(0) {
	mhStopEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	WLog_SetLogLevel(logger_RPCEngine, WLOG_ERROR);
}
RpcEngine::~RpcEngine() {
	google::protobuf::ShutdownProtobufLibrary();

}

HANDLE RpcEngine::createServerPipe(const char* endpoint) {
	HANDLE hNamedPipe;

	hNamedPipe = CreateNamedPipe(endpoint, PIPE_ACCESS_DUPLEX,
	PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
	PIPE_UNLIMITED_INSTANCES, PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, 0, NULL);

	if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE)) {
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "creating namedpipe failed");
		return NULL;
	}

	return hNamedPipe;
}

int RpcEngine::startEngine() {

	mhServerThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE) RpcEngine::listenerThread, (void*) this,
			CREATE_SUSPENDED, NULL);

	ResumeThread(mhServerThread);

	return CLIENT_SUCCESS;
}

int RpcEngine::stopEngine() {
	if (mhServerThread) {
		SetEvent(mhStopEvent);
		WaitForSingleObject(mhServerThread,INFINITE);
		CloseHandle(mhServerThread);
		mhServerThread = NULL;
	}
	return CLIENT_SUCCESS;
}

int RpcEngine::createServerPipe() {
	mhServerPipe = createServerPipe("\\\\.\\pipe\\FreeRDS_SessionManager");

	if (!mhServerPipe) {
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "Could not create named pipe \\\\.\\pipe\\FreeRDS_SessionManager");
		return CLIENT_ERROR;
	}

}

void* RpcEngine::listenerThread(void* arg) {
	RpcEngine* engine;

	engine = (RpcEngine*) arg;
	WLog_Print(logger_RPCEngine, WLOG_TRACE, "started RPC listener Thread");

	while (1) {

		if (!engine->createServerPipe()) {
			break;
		}

		HANDLE clientPipe = engine->acceptClient();

		if (!clientPipe)
			break;

		if (engine->serveClient() == CLIENT_ERROR ) {
			break;
		}
		engine->resetStatus();

	}

	return NULL;
}

HANDLE RpcEngine::acceptClient() {

	if (!mhServerPipe) {
		return NULL;
	}
	DWORD status;
	DWORD nCount;
	HANDLE events[2];

	nCount = 0;
	events[nCount++] = mhStopEvent;
	events[nCount++] = mhServerPipe;

	status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);
/*	if (status == WAIT_OBJECT_0) {
		return NULL;
	} else if (status == WAIT_OBJECT_0 +1) {*/
	if (WaitForSingleObject(mhStopEvent, 0) == WAIT_OBJECT_0) {
		WLog_Print(logger_RPCEngine, WLOG_TRACE, "got shutdown signal");
		return NULL;
	}
	if (WaitForSingleObject(mhServerPipe, 0) == WAIT_OBJECT_0) {
		BOOL fConnected;
		DWORD dwPipeMode;

		fConnected = ConnectNamedPipe(mhServerPipe, NULL);

		if (!fConnected)
			fConnected = (GetLastError() == ERROR_PIPE_CONNECTED);

		if (!fConnected) {
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

int RpcEngine::read() {
	if (mPacktLength > 0) {
		return readPayload();
	} else {
		return readHeader();
	}
}

int RpcEngine::readHeader() {

	DWORD lpNumberOfBytesRead = 0;
	BOOL fSuccess;

	fSuccess = ReadFile(mhClientPipe, mHeaderBuffer + mHeaderRead,
			4 - mHeaderRead, &lpNumberOfBytesRead, NULL);

	if (!fSuccess || (lpNumberOfBytesRead == 0)) {
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "error reading");
		return CLIENT_DISCONNECTED;
	}

	mHeaderRead += lpNumberOfBytesRead;
	if (mHeaderRead == 4) {
		mPacktLength = ntohl(*(DWORD *)mHeaderBuffer);
		WLog_Print(logger_RPCEngine, WLOG_TRACE, "header read, packet size %d",mPacktLength);
	}
	return CLIENT_SUCCESS;
}

int RpcEngine::readPayload() {

	DWORD lpNumberOfBytesRead = 0;
	BOOL fSuccess;

	fSuccess = ReadFile(mhClientPipe, mPayloadBuffer + mPayloadRead,
			mPacktLength - mPayloadRead, &lpNumberOfBytesRead,
			NULL);

	if (!fSuccess || (lpNumberOfBytesRead == 0)) {
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "error reading payload");
		return CLIENT_DISCONNECTED;
	}

	mPayloadRead += lpNumberOfBytesRead;
	return CLIENT_SUCCESS;
}

int RpcEngine::processData() {
	mpbRPC.Clear();
	mpbRPC.ParseFromArray(mPayloadBuffer, mPayloadRead);

	if (mpbRPC.isresponse()) {
		// TODO handle calls which are send out by us

	} else {

		uint32_t callID = mpbRPC.tag();
		uint32_t callType = mpbRPC.msgtype();
		callNS::Call* createdCall = CALL_FACTORY.createClass(callType);
		if (createdCall == NULL) {
			// call not found ... send error
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "no registered class for calltype=%d",callType);
			sendError(callID, callType);
			delete createdCall;
			return CLIENT_ERROR;
		}
		if ( createdCall->getDerivedType() == 1) {
			// we got an CallIn object ... so handle it
			callNS::CallIn* createdCallIn = (callNS::CallIn*)createdCall;
			createdCallIn->setEncodedRequest(mpbRPC.payload());
			createdCallIn->setTag(callID);
			WLog_Print(logger_RPCEngine, WLOG_TRACE, "call upacked for callType=%d and callID=%d",callType,callID);
			// call the implementation ...
			createdCallIn->decodeRequest();
			if (!createdCallIn->doStuff()) {
				createdCallIn->encodeResponse();
			}
			// send the result
			send(createdCall);
			delete createdCall;
		} else {
			WLog_Print(logger_RPCEngine, WLOG_ERROR, "no registered class for calltype=%d",callType);
			sendError(callID, callType);
			delete createdCall;
			return CLIENT_ERROR;
		}
		return CLIENT_SUCCESS;
	}
}

int RpcEngine::send(freerds::sessionmanager::call::Call * call) {

	DWORD lpNumberOfBytesWritten;
	std::string serialized;


	if (call->getDerivedType() == 1) {
		// this is a CallIn
		callNS::CallIn * callIn = (callNS::CallIn *)call;
		// create answer
		mpbRPC.Clear();
		mpbRPC.set_isresponse(true);
		mpbRPC.set_tag(callIn->getTag());
		mpbRPC.set_msgtype(callIn->getCallType());
		if (call->getResult()) {
			WLog_Print(logger_RPCEngine, WLOG_TRACE, "call for callType=%d and callID=%d failed, sending error response",callIn->getCallType(),callIn->getTag());
			mpbRPC.set_status(RPCBase_RPCSTATUS_FAILED);
			std::string errordescription = callIn->getErrorDescription();
			if (errordescription.size() > 0) {
				mpbRPC.set_errordescription(errordescription);
			}
		} else {
			WLog_Print(logger_RPCEngine, WLOG_TRACE, "call for callType=%d and callID=%d success, sending response",callIn->getCallType(),callIn->getTag());
			mpbRPC.set_status(RPCBase_RPCSTATUS_SUCCESS);
			mpbRPC.set_payload(callIn->getEncodedResponse());
		}

		mpbRPC.SerializeToString(&serialized);
	} else if (call->getDerivedType() == 2) {
		// this is a CallOut
	}



	return sendInternal(serialized);

}

int RpcEngine::sendError(uint32_t callID, uint32_t callType) {

	std::string serialized;

	mpbRPC.Clear();
	mpbRPC.set_isresponse(true);
	mpbRPC.set_tag(callID);
	mpbRPC.set_msgtype(callType);
	mpbRPC.set_status(RPCBase_RPCSTATUS_NOTFOUND);

	mpbRPC.SerializeToString(&serialized);

	return sendInternal(serialized);

}

int RpcEngine::sendInternal(std::string data) {
	DWORD lpNumberOfBytesWritten;
	DWORD messageSize = htonl(data.size());

	bool fSuccess = WriteFile(mhClientPipe, &messageSize,
			4, &lpNumberOfBytesWritten, NULL);

	if (!fSuccess || (lpNumberOfBytesWritten == 0)) {
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending");
		return CLIENT_ERROR;
	}

	fSuccess = WriteFile(mhClientPipe, data.c_str(),
			data.size(), &lpNumberOfBytesWritten, NULL);

	if (!fSuccess || (lpNumberOfBytesWritten == 0)) {
		WLog_Print(logger_RPCEngine, WLOG_ERROR, "error sending");
		return -1;
	}
}

void RpcEngine::resetStatus() {
	mPacktLength = 0;
	mHeaderRead = 0;
	mPayloadRead = 0;
}

int RpcEngine::serveClient() {

	DWORD status;
	DWORD nCount;
	HANDLE events[2];

	nCount = 0;
	events[nCount++] = mhStopEvent;
	events[nCount++] = mhClientPipe;

	int retValue = 0;

	while (1) {
		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		/*if (status == WAIT_OBJECT_0) {
			retValue = CLIENT_ERROR;
			break;
		} else if (status == WAIT_OBJECT_0 + 1) {*/


		if (WaitForSingleObject(mhStopEvent, 0) == WAIT_OBJECT_0) {
			retValue = CLIENT_ERROR;
			break;
		}

		if (WaitForSingleObject(mhClientPipe, 0) == WAIT_OBJECT_0) {
			retValue = read();
			if (retValue) {
				break;
			}
			// process the data
			if (mPayloadRead == mPacktLength) {
				processData();
				resetStatus();
			}

		}
	}

	return retValue;
}

}
}
