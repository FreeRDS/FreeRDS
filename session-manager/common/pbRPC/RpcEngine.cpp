#include "RpcEngine.h"

#include <winpr/pipe.h>
#include <winpr/thread.h>
#include <call/CallFactory.h>
#include <arpa/inet.h>

namespace freerds {
namespace pbrpc {

#define CLIENT_DISCONNECTED 2
#define CLIENT_ERROR -1
#define CLIENT_SUCCESS 0

RpcEngine::RpcEngine() :
		mPacktLength(0), mHeaderRead(0), mPayloadRead(0) {
	mhStopEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
}
RpcEngine::~RpcEngine() {

}

HANDLE RpcEngine::createServerPipe(const char* endpoint) {
	HANDLE hNamedPipe;

	hNamedPipe = CreateNamedPipe(endpoint, PIPE_ACCESS_DUPLEX,
	PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
	PIPE_UNLIMITED_INSTANCES, PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, 0, NULL);

	if ((!hNamedPipe) || (hNamedPipe == INVALID_HANDLE_VALUE)) {
		fprintf(stderr, "CreateNamedPipe failure\n");
		return NULL;
	}

	return hNamedPipe;
}

int RpcEngine::startEngine() {

	mhServerPipe = createServerPipe("\\\\.\\pipe\\FreeRDS_SessionManager");

	if (!mhServerPipe)
		return CLIENT_ERROR;

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
		mhServerThread = NULL;
	}
	return CLIENT_SUCCESS;
}

void* RpcEngine::listenerThread(void* arg) {
	RpcEngine* engine;

	engine = (RpcEngine*) arg;

	while (1) {
		HANDLE clientPipe = engine->acceptClient();

		if (!clientPipe)
			break;

		engine->serveClient();

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

	if (WaitForSingleObject(mhStopEvent, 0) == WAIT_OBJECT_0) {
		return NULL;
	}
	if (WaitForSingleObject(mhServerPipe, 0) == WAIT_OBJECT_0) {
		BOOL fConnected;
		DWORD dwPipeMode;

		fConnected = ConnectNamedPipe(mhServerPipe, NULL);

		if (!fConnected)
			fConnected = (GetLastError() == ERROR_PIPE_CONNECTED);

		if (!fConnected) {
			return NULL;
		}

		mhClientPipe = mhServerPipe;

		dwPipeMode = PIPE_WAIT;
		SetNamedPipeHandleState(mhClientPipe, &dwPipeMode, NULL, NULL);

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
		printf("Server NamedPipe readHeader failure\n");
		return CLIENT_DISCONNECTED;
	}

	mHeaderRead += lpNumberOfBytesRead;
	if (mHeaderRead == 4) {
		mPacktLength = ntohl(*(DWORD *)mHeaderBuffer);;
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
		printf("Server NamedPipe readPayload failure\n");
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
			sendError(callID, callType);
			return CLIENT_ERROR;
		}
		createdCall->setEncodedRequest(mpbRPC.payload());
		createdCall->setTag(callID);

		// call the implementation ...
		createdCall->decodeRequest();
		if (!createdCall->doStuff()) {
			createdCall->encodeResponse();
		}
		// send the result
		send(createdCall);
		return CLIENT_SUCCESS;
	}
}

int RpcEngine::send(freerds::sessionmanager::call::Call * call) {

	DWORD lpNumberOfBytesWritten;
	std::string serialized;

	// create answer
	mpbRPC.Clear();
	mpbRPC.set_isresponse(true);
	mpbRPC.set_tag(call->getTag());
	mpbRPC.set_msgtype(call->getCallType());
	if (call->getResult()) {
		mpbRPC.set_status(RPCBase_RPCSTATUS_FAILED);
		std::string errordescription = call->getErrorDescription();
		if (errordescription.size() > 0) {
			mpbRPC.set_errordescription(call->getErrorDescription());
		}
	} else {
		mpbRPC.set_status(RPCBase_RPCSTATUS_SUCCESS);
		mpbRPC.set_payload(call->getEncodedResponse());
	}

	mpbRPC.SerializeToString(&serialized);

	bool fSuccess = WriteFile(mhClientPipe, serialized.c_str(),
			serialized.size(), &lpNumberOfBytesWritten, NULL);

	if (!fSuccess || (lpNumberOfBytesWritten == 0)) {
		printf("Server NamedPipe send failure\n");
		return CLIENT_ERROR;
	}

}

int RpcEngine::sendError(uint32_t callID, uint32_t callType) {

	DWORD lpNumberOfBytesWritten;
	std::string serialized;

	mpbRPC.Clear();
	mpbRPC.set_isresponse(true);
	mpbRPC.set_tag(callID);
	mpbRPC.set_msgtype(callType);
	mpbRPC.set_status(RPCBase_RPCSTATUS_NOTFOUND);

	mpbRPC.SerializeToString(&serialized);

	bool fSuccess = WriteFile(mhClientPipe, serialized.c_str(),
			serialized.size(), &lpNumberOfBytesWritten, NULL);

	if (!fSuccess || (lpNumberOfBytesWritten == 0)) {
		printf("Server NamedPipe sendError failure\n");
		return -1;
	}

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
				mPacktLength = 0;
				mHeaderRead = 0;
				mPayloadRead = 0;
			}

		}
	}

	return retValue;
}

}
}
