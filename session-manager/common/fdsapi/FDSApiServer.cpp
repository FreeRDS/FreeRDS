/**
 *  FDSApiServer
 *
 *  Starts and stops the thrift server.
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

#include "FDSApiServer.h"

#include <appcontext/ApplicationContext.h>
#include "FDSApiHandler.h"

#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <winpr/thread.h>
#include <winpr/wlog.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

using boost::shared_ptr;


namespace freerds{
	namespace sessionmanager{
		namespace fdsapi {

		static wLog * logger_FDSApiServer = WLog_Get("freerds.sessionmanager.fdsapiserver");

		FDSApiServer::FDSApiServer()
		{
			if (!InitializeCriticalSectionAndSpinCount(&mCSection,
					0x00000400) )
			{
				WLog_Print(logger_FDSApiServer, WLOG_ERROR, "FDSApiServer: InitializeCriticalSectionAndSpinCount failed!");
			}
			mServerThread = NULL;
			mPort = 9091;
		}

		FDSApiServer::~FDSApiServer()
		{
		}

		void FDSApiServer::setServer(boost::shared_ptr<apache::thrift::server::TServer> server)
		{
			mServer = server;
		}

		void FDSApiServer::setPort(DWORD port)
		{
			mPort = port;
		}

		DWORD FDSApiServer::getPort()
		{
			return mPort;
		}

		CRITICAL_SECTION* FDSApiServer::getCritSection()
		{
			return &mCSection;
		}


		void FDSApiServer::serverThread(void * param)
		{
			FDSApiServer * server = (FDSApiServer *) param;

			shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
			shared_ptr<FDSApiHandler> handler(new FDSApiHandler());
			shared_ptr<TProcessor> processor(new fdsapiProcessor(handler));
			shared_ptr<TServerTransport> serverTransport(new TServerSocket(server->getPort()));
			shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());

			shared_ptr<TServer> thriftServer(new TSimpleServer(processor, serverTransport, transportFactory, protocolFactory));
			server->setServer(thriftServer);
			LeaveCriticalSection(server->getCritSection());
			WLog_Print(logger_FDSApiServer, WLOG_INFO, "FDSApiServer started on port %d",server->getPort());
			thriftServer->serve();
			WLog_Print(logger_FDSApiServer, WLOG_INFO, "FDSApiServer stopped.");
		 }

		 void FDSApiServer::startFDSApi()
		 {
			EnterCriticalSection(&mCSection);
			mServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) FDSApiServer::serverThread, (void*) this, 0, NULL);
			ResumeThread(mServerThread);
			EnterCriticalSection(&mCSection);
			LeaveCriticalSection(&mCSection);     }

		 void FDSApiServer::stopFDSApi()
		 {
			 if (mServer != NULL) {
				WLog_Print(logger_FDSApiServer, WLOG_INFO, "Stopping FDSApiServer ...")
				 mServer->stop();
				 WaitForSingleObject(mServerThread,INFINITE);
				 CloseHandle(mServerThread);
				 mServerThread = NULL;
				 mServer.reset();
			 }
		 }

		}
	}
}


