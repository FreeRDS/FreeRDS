/**
 * Connection class
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

#include <winpr/crt.h>
#include <winpr/wlog.h>
#include <winpr/pipe.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/environment.h>

#include <iostream>
#include <sstream>

#include <session/ApplicationContext.h>
#include <module/AuthModule.h>
#include <utils/CSGuard.h>

#include "Connection.h"

namespace freerds
{
	static wLog* logger_Connection = WLog_Get("freerds.Connection");

	Connection::Connection(DWORD connectionId)
		: m_ConnectionId(connectionId), m_SessionId(0),
		  m_AuthStatus(-1), m_About2SwitchSessionId(0)
	{
		if (!InitializeCriticalSectionAndSpinCount(&m_CSection, 0x00000400))
		{
			 WLog_Print(logger_Connection, WLOG_FATAL, "cannot init SessionStore critical section!");
		}

		ZeroMemory(&m_ClientInformation, sizeof(m_ClientInformation));
	}

	Connection::~Connection()
	{

	}

	std::string Connection::getDomain() {
		return m_Domain;
	}

	std::string Connection::getUserName() {
		return m_Username;
	}

	void Connection::setSessionId(UINT32 sessionId) {
		m_SessionId = sessionId;
	}

	UINT32 Connection::getSessionId() {
		return m_SessionId;
	}

	UINT32 Connection::getConnectionId() {
		return m_ConnectionId;
	}

	int Connection::authenticateUser(std::string username, std::string domain, std::string password)
	{
		CSGuard guard(&m_CSection);

		if (m_AuthStatus == 0) {
			// a Connection can only be authorized once
			return -1;
		}

		std::string authModule;

		if (!APP_CONTEXT.getPropertyManager()->getPropertyString("auth.module", authModule)) {
			authModule = "PAM";
		}

		AuthModule* auth = AuthModule::loadFromName(authModule);

		if (!auth) {
			return 1;
		}

		m_AuthStatus = auth->logonUser(username, domain, password);

		delete auth;

		if (m_AuthStatus == 0)
		{
			m_Username = username;
			m_Domain = domain;
		}

		return m_AuthStatus;
	}

	pCLIENT_INFORMATION Connection::getClientInformation() {
		return &m_ClientInformation;
	}

	UINT32 Connection::getAbout2SwitchSessionId() {
		return m_About2SwitchSessionId;
	}

	void Connection::setAbout2SwitchSessionId(UINT32 switchSessionId) {
		m_About2SwitchSessionId = switchSessionId;
	}
}

