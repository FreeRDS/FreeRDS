/**
 * Connection store class
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


#ifndef __CONNECTION_STORE_H_
#define __CONNECTION_STORE_H_

#include <config.h>

#include "Connection.h"

#include <string>
#include <winpr/synch.h>
#include <map>

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{

		typedef std::map<long , ConnectionPtr> TConnectionMap;
		typedef std::pair<long, ConnectionPtr> TConnectionPair;


		class ConnectionStore
		{
		public:
			ConnectionStore();
			~ConnectionStore();

			ConnectionPtr getOrCreateConnection(long connectionID);
			ConnectionPtr getConnection(long connectionID);
			int removeConnection(long connectionID);

			long getConnectionIdForSessionId(long mSessionId);

			void reset();

		private:
			TConnectionMap mConnectionMap;
			CRITICAL_SECTION mCSection;
		};
		}
	}
}

namespace sessionNS = freerds::sessionmanager::session;

#endif //__CONNECTION_STORE_H_
