#ifndef RPCENGINE_H_
#define RPCENGINE_H_

#include <winpr/synch.h>
#include <pbRPC.pb.h>
#include <call/Call.h>



#define PIPE_BUFFER_SIZE	0xFFFF

namespace freeRDS{
	namespace pbRPC{

		class RpcEngine{
		public:
			RpcEngine();
			~RpcEngine();

			int startEngine();

			HANDLE acceptClient();
			int serveClient();

		private:
			HANDLE createServerPipe(const char* endpoint);
			static void* listenerThread(void* arg);
			int read();
			int readHeader();
			int readPayload();
			int processData();
			int send(freeRDS::sessionmanager::call::Call * call);
			int sendError(uint32_t callID, uint32_t callType);


		private:
			HANDLE mhClientPipe;
			HANDLE mhServerPipe;
			HANDLE mhServerThread;

			HANDLE mhStopEvent;

			BOOL mHasHeaders;

			DWORD mHeaderRead;
			BYTE mHeaderBuffer[4];

			DWORD mPayloadRead;
			BYTE mPayloadBuffer[PIPE_BUFFER_SIZE];

			RPCBase mpbRPC;

		};

	}
}

namespace pbRPC = freeRDS::pbRPC;



#endif /* RPCENGINE_H_ */
