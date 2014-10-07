/**
 * Session store class
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

#include <winpr/wlog.h>

#include "SessionStore.h"

#include <utils/CSGuard.h>

namespace freerds
{
	static wLog* logger_SessionStore = WLog_Get("freerds.SessionStore");

	SessionStore::SessionStore()
	{
		if (!InitializeCriticalSectionAndSpinCount(&m_CSection, 0x00000400))
		{
			 WLog_Print(logger_SessionStore, WLOG_FATAL, "cannot init SessionStore critical section!");
		}
		m_NextSessionId = 1;
	}

	SessionStore::~SessionStore()
	{
		DeleteCriticalSection(&m_CSection);
	}

	SessionPtr SessionStore::getSession(UINT32 sessionId)
	{
		CSGuard guard(&m_CSection);

		if (m_SessionMap.find(sessionId) != m_SessionMap.end()) {
			return m_SessionMap[sessionId];
		}

		return SessionPtr();
	}

	SessionPtr SessionStore::createSession()
	{
		CSGuard guard(&m_CSection);
		SessionPtr session(new Session(m_NextSessionId++));
		m_SessionMap[session->getSessionId()] = session;
		return session;
	}

	SessionPtr SessionStore::getFirstSessionUserName(std::string username,std::string domain)
	{
		CSGuard guard(&m_CSection);

		SessionPtr session;
		TSessionMap::iterator iter;

		for (iter = m_SessionMap.begin(); iter != m_SessionMap.end(); iter++)
		{
			if ((iter->second->getUserName().compare(username) == 0) &&
					(iter->second->getDomain().compare(domain) == 0))
			{
				session = iter->second;
				break;
			}
		}

		return session;
	}

	SessionPtr SessionStore::getFirstDisconnectedSessionUserName(
			std::string username, std::string domain)
	{
		CSGuard guard(&m_CSection);

		SessionPtr session;
		TSessionMap::iterator iter;

		for (iter = m_SessionMap.begin(); iter != m_SessionMap.end();iter++)
		{
			if((iter->second->getUserName().compare(username) == 0) &&
					(iter->second->getDomain().compare(domain) == 0))
			{
				if (iter->second->getConnectState() == WTSDisconnected) {
					session = iter->second;
					break;
				}
			}
		}

		return session;
	}

	int SessionStore::removeSession(UINT32 sessionId)
	{
		CSGuard guard(&m_CSection);
		m_SessionMap.erase(sessionId);
		return 0;
	}

	std::list<SessionPtr> SessionStore::getAllSessions()
	{
		CSGuard guard(&m_CSection);
		std::list<SessionPtr> list;

		for (TSessionMap::const_iterator it = m_SessionMap.begin(); it != m_SessionMap.end(); ++it) {
			list.push_back(it->second);
		}

		return list;
	}
}

