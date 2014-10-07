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
	static wLog* logger_ConnectionStore = WLog_Get("freerds.ConnectionStore");

	ConnectionStore::ConnectionStore()
	{
		if (!InitializeCriticalSectionAndSpinCount(&m_CSection, 0x00000400))
		{
			 WLog_Print(logger_ConnectionStore, WLOG_FATAL, "cannot init ConnectionStore critical section!");
		}
	}

	ConnectionStore::~ConnectionStore()
	{
		DeleteCriticalSection(&m_CSection);
	}

	ConnectionPtr ConnectionStore::getOrCreateConnection(UINT32 connectionId)
	{
		CSGuard guard(&m_CSection);

		if (m_ConnectionMap.find(connectionId) != m_ConnectionMap.end())
		{
			return m_ConnectionMap[connectionId];
		}
		else
		{
			ConnectionPtr connection = ConnectionPtr(new Connection(connectionId));
			m_ConnectionMap[connectionId] = connection;
			return connection;
		}
	}

	ConnectionPtr ConnectionStore::getConnection(UINT32 connectionId)
	{
		CSGuard guard(&m_CSection);

		if (m_ConnectionMap.find(connectionId) != m_ConnectionMap.end()) {
			return m_ConnectionMap[connectionId];
		}

		return ConnectionPtr();
	}

	int ConnectionStore::removeConnection(UINT32 connectionId)
	{
		CSGuard guard(&m_CSection);
		m_ConnectionMap.erase(connectionId);
		return 0;
	}

	UINT32 ConnectionStore::getConnectionIdForSessionId(UINT32 sessionId)
	{
		CSGuard guard(&m_CSection);

		UINT32 connectionId = 0;

		TConnectionMap::iterator iter;

		for (iter = m_ConnectionMap.begin(); iter != m_ConnectionMap.end();iter++)
		{
			if ((iter->second->getSessionId() == sessionId) ||
					(iter->second->getAbout2SwitchSessionId() == sessionId))
			{
				connectionId = iter->first;
				break;
			}
		}

		return connectionId;
	}

	void ConnectionStore::reset()
	{
		CSGuard guard(&m_CSection);
		m_ConnectionMap.clear();
	}
}

