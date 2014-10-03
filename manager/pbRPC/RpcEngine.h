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

#ifndef RPCENGINE_H_
#define RPCENGINE_H_

#include <winpr/synch.h>
#include <pbRPC.pb.h>
#include <call/Call.h>
#include <call/CallOut.h>
#include <list>

#define PIPE_BUFFER_SIZE	0xFFFF

namespace freerds
{
	namespace pbrpc
	{
		class RpcEngine
		{
		public:
			RpcEngine();
			~RpcEngine();

			int startEngine();
			int stopEngine();

			HANDLE acceptClient();
			int serveClient();
			void resetStatus();

		private:
			int createServerPipe(void);
			HANDLE createServerPipe(const char* endpoint);
			static void* listenerThread(void* arg);
			int read();
			int readHeader();
			int readPayload();
			int processData();
			int send(freerds::call::Call * call);
			int sendError(uint32_t callID, uint32_t callType);
			int sendInternal(std::string data);
			int processOutgoingCall(freerds::call::Call* call);

		private:
			HANDLE mhClientPipe;
			HANDLE mhServerPipe;
			HANDLE mhServerThread;

			HANDLE mhStopEvent;

			DWORD mPacktLength;

			DWORD mHeaderRead;
			BYTE mHeaderBuffer[4];

			DWORD mPayloadRead;
			BYTE mPayloadBuffer[PIPE_BUFFER_SIZE];

			RPCBase mpbRPC;
			std::list<callNS::CallOut*> mAnswerWaitingQueue;

			long mNextOutCall;
		};
	}
}

namespace pbRPC = freerds::pbrpc;

#endif /* RPCENGINE_H_ */
