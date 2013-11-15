/**
 * Session class
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

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <config.h>
#include <string>
#include <list>

#include <winpr/crt.h>
#include <winpr/wtsapi.h>

#include <freerds/modules.h>

#include <freerds/freerds.h>
#include <freerds/module_connector.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{
		class Connection
		{
		public:
			Connection(DWORD connectionId);
			~Connection();

			HANDLE createServerPipe();
			HANDLE connectClientPipe(std::string clientPipeName);

			std::string getServerPipeName();
			std::string getClientPipeName();

			static Connection* create();

		private:
			DWORD mConnectionId;
			HANDLE mListenPipe;
			HANDLE mServerPipe;
			HANDLE mClientPipe;
			bool mServerPipeConnected;
			bool mClientPipeConnected;
			std::string mServerPipeName;
			std::string mClientPipeName;
		};
		}
	}
}

namespace sessionNS = freerds::sessionmanager::session;

#endif // CONNECTION_H_
