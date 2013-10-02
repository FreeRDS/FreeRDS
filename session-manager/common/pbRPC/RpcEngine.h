#ifndef RPCENGINE_H_
#define RPCENGINE_H_

#include <winpr/synch.h>
#include <pbRPC.pb.h>
#include <call/Call.h>



#define PIPE_BUFFER_SIZE	0xFFFF

namespace freerds{
	namespace pbrpc{

		class RpcEngine{
		public:
			RpcEngine();
			~RpcEngine();

			int startEngine();
			int stopEngine();

			HANDLE acceptClient();
			int serveClient();

		private:
			HANDLE createServerPipe(const char* endpoint);
			static void* listenerThread(void* arg);
			int read();
			int readHeader();
			int readPayload();
			int processData();
			int send(freerds::sessionmanager::call::Call * call);
			int sendError(uint32_t callID, uint32_t callType);
			int sendInternal(std::string data);


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

		};

	}
}

namespace pbRPC = freerds::pbrpc;



#endif /* RPCENGINE_H_ */
