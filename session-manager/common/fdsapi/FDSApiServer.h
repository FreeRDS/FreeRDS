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


#ifndef FDSAPISERVER_H_
#define FDSAPISERVER_H_

#include <winpr/synch.h>
#include <winpr/thread.h>

#include <freerds/rpc.h>

#include <boost/shared_ptr.hpp>
#include <thrift/server/TServer.h>

namespace freerds{
	namespace sessionmanager{
		namespace fdsapi{

		class FDSApiServer {
		 public:
			FDSApiServer();
			virtual ~FDSApiServer();

			static void serverThread( void * parameter);
			void startFDSApi();
			void stopFDSApi();

			void setPort(DWORD port);
			DWORD getPort();

			void setServer(boost::shared_ptr<apache::thrift::server::TServer> server);
			CRITICAL_SECTION * getCritSection();

		 private:
			CRITICAL_SECTION mCSection;
			boost::shared_ptr<apache::thrift::server::TServer> mServer;
			HANDLE mServerThread;
			DWORD mPort;

			rdsRpc* mRpcServer;
		};

		}
	}
}
namespace fdsapiNS = freerds::sessionmanager::fdsapi;
#endif /* FDSAPISERVER_H_ */
