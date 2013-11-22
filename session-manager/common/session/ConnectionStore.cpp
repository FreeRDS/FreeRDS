/**
 * Connection store class
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

#include "ConnectionStore.h"
#include <winpr/wlog.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{
			static wLog * logger_ConnectionStore = WLog_Get("freerds.SessionManager.session.connectionstore");

			ConnectionStore::ConnectionStore()
			{
				if (!InitializeCriticalSectionAndSpinCount(&mCSection, 0x00000400))
				{
					 WLog_Print(logger_ConnectionStore, WLOG_FATAL, "cannot init ConnectionStore critical section!");
				}
			}

			ConnectionStore::~ConnectionStore()
			{
				DeleteCriticalSection(&mCSection);
			}

			Connection* ConnectionStore::getOrCreateConnection(long connectionID)
			{
				EnterCriticalSection(&mCSection);
				Connection* connection = mConnectionMap[connectionID];
				if (connection == NULL) {
					connection = new Connection(connectionID);
					mConnectionMap[connectionID] = connection;

				}
				LeaveCriticalSection(&mCSection);
				return connection;
			}

			Connection* ConnectionStore::getConnection(long connectionID)
			{
				EnterCriticalSection(&mCSection);
				Connection* connection = mConnectionMap[connectionID];
				LeaveCriticalSection(&mCSection);
				return connection;
			}


			int ConnectionStore::removeConnection(long connectionID)
			{
				EnterCriticalSection(&mCSection);
				Connection * connection = mConnectionMap[connectionID];
				mConnectionMap.erase(connectionID);
				delete connection;
				LeaveCriticalSection(&mCSection);
				return 0;
			}
		}
	}
}
