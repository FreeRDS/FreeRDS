/**
 * Session store class
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

#include "SessionStore.h"
#include <winpr/wlog.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{
			static wLog * logger_SessionStore = WLog_Get("freerds.SessionManager.session.sessionstore");

			SessionStore::SessionStore()
			{
				if (!InitializeCriticalSectionAndSpinCount(&mCSection, 0x00000400))
				{
					 WLog_Print(logger_SessionStore, WLOG_FATAL, "cannot init SessionStore critical section!");
				}
				mNextSessionId = 1;
			}

			SessionStore::~SessionStore()
			{
				DeleteCriticalSection(&mCSection);
			}

			Session* SessionStore::getSession(long sessionId)
			{
				EnterCriticalSection(&mCSection);
				Session* session = mSessionMap[sessionId];
				LeaveCriticalSection(&mCSection);
				return session;
			}

			Session* SessionStore::createSession()
			{
				EnterCriticalSection(&mCSection);
				Session * session = new Session(mNextSessionId++);
				mSessionMap[session->getSessionID()] = session;
				LeaveCriticalSection(&mCSection);
				return session;
			}

			Session* SessionStore::getFirstSessionUserName(std::string username,std::string domain)
			{
				EnterCriticalSection(&mCSection);

				Session* session = NULL;
				TSessionMap::iterator iter;

				for (iter = mSessionMap.begin(); iter != mSessionMap.end();iter++)
				{
					if((iter->second->getUserName().compare(username) == 0) &&
							(iter->second->getDomain().compare(domain) == 0))
					{
						session = iter->second;
						break;
					}
				}

				LeaveCriticalSection(&mCSection);

				return session;
			}

			int SessionStore::removeSession(long sessionId)
			{
				EnterCriticalSection(&mCSection);
				Session * session = mSessionMap[sessionId];
				mSessionMap.erase(sessionId);
				delete session;
				LeaveCriticalSection(&mCSection);
				return 0;
			}
		}
	}
}
