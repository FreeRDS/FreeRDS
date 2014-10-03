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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ConnectionStore.h"
#include <winpr/wlog.h>
#include <utils/CSGuard.h>

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

			ConnectionPtr ConnectionStore::getOrCreateConnection(long connectionID)
			{
				CSGuard guard(&mCSection);
				if (mConnectionMap.find(connectionID) != mConnectionMap.end()) {
					return mConnectionMap[connectionID];
				} else {
					ConnectionPtr connection = ConnectionPtr(new Connection(connectionID));
					mConnectionMap[connectionID] = connection;
					return connection;
				}
			}

			ConnectionPtr ConnectionStore::getConnection(long connectionID)
			{
				CSGuard guard(&mCSection);
				if (mConnectionMap.find(connectionID) != mConnectionMap.end()) {
					return mConnectionMap[connectionID];
				}
				return ConnectionPtr();
			}


			int ConnectionStore::removeConnection(long connectionID)
			{
				CSGuard guard(&mCSection);
				mConnectionMap.erase(connectionID);
				return 0;
			}

			long ConnectionStore::getConnectionIdForSessionId(long mSessionId) {
				CSGuard guard(&mCSection);

				long connectionId = 0;

				TConnectionMap::iterator iter;

				for (iter = mConnectionMap.begin(); iter != mConnectionMap.end();iter++)
				{
					if((iter->second->getSessionId() == mSessionId) || (iter->second->getAbout2SwitchSessionId() == mSessionId))
					{
						connectionId = iter->first;
						break;
					}
				}

				return connectionId;
			}

			void ConnectionStore::reset() {
				CSGuard guard(&mCSection);
				mConnectionMap.clear();
			}


		}
	}
}

