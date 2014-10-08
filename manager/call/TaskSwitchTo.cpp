/**
 * Task for switching a Module
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

#include "TaskSwitchTo.h"
#include "CallOutSwitchTo.h"

#include <session/ApplicationContext.h>

namespace freerds
{
	static wLog* logger_taskSwitchTo = WLog_Get("freerds.TaskSwitchTo");

	void TaskSwitchTo::run()
	{
		CallOutSwitchTo switchToCall;

		switchToCall.setServiceEndpoint(m_ServiceEndpoint);
		switchToCall.setConnectionId(m_ConnectionId);

		APP_CONTEXT.getRpcOutgoingQueue()->addElement(&switchToCall);
		WaitForSingleObject(switchToCall.getAnswerHandle(), INFINITE);

		if (switchToCall.getResult() != 0) {
			WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo answer: RPC error %d!", switchToCall.getResult());
			return cleanUpOnError();
		}

		if (switchToCall.decodeResponse()) {
			//
			WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: decoding of switchto answer failed!");
			return cleanUpOnError();
		}
		if (!switchToCall.isSuccess()) {
			WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: switching in FreeRDS failed!");
			return cleanUpOnError();
		}
		SessionPtr currentSession;

		if (m_OldSessionId != 0)
		{
			currentSession = APP_CONTEXT.getSessionStore()->getSession(m_OldSessionId);

			if (currentSession) {
				currentSession->stopModule();
				APP_CONTEXT.getSessionStore()->removeSession(currentSession->getSessionId());
				WLog_Print(logger_taskSwitchTo, WLOG_INFO, "TaskSwitchTo: session with sessionId %d was stopped!", m_OldSessionId);
			}
			else
			{
				WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: no session was found for sessionId %d!", m_OldSessionId);
			}
		}
		else
		{
			WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: no oldSessionId was set!");
		}

		ConnectionPtr connection = APP_CONTEXT.getConnectionStore()->getConnection(m_ConnectionId);

		if (connection) {
			connection->setSessionId(m_NewSessionId);
			connection->setAbout2SwitchSessionId(0);
		}

		return;
	}

	void TaskSwitchTo::setConnectionId(UINT32 connectionId) {
		m_ConnectionId = connectionId;
	}

	void TaskSwitchTo::setServiceEndpoint(std::string serviceEndpoint) {
		m_ServiceEndpoint = serviceEndpoint;
	}

	void TaskSwitchTo::setOldSessionId(UINT32 sessionId) {
		m_OldSessionId = sessionId;
	}

	void TaskSwitchTo::setNewSessionId(UINT32 sessionId) {
		m_NewSessionId = sessionId;
	}

	void TaskSwitchTo::cleanUpOnError()
	{
		SessionPtr currentSession = APP_CONTEXT.getSessionStore()->getSession(m_NewSessionId);

		if (currentSession)
		{
			if (currentSession->getConnectState() == WTSActive)
			{
				// this was a new session for the connection, remove it
				currentSession->stopModule();
				APP_CONTEXT.getSessionStore()->removeSession(currentSession->getSessionId());
				WLog_Print(logger_taskSwitchTo, WLOG_INFO, "TaskSwitchTo: cleaning up session with sessionId %d", m_NewSessionId);
			}
			else if (currentSession->getConnectState() == WTSConnectQuery)
			{
				// was a previous disconnected session
				currentSession->setConnectState(WTSDisconnected);
			}
		}
		else
		{
			WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: no session was found for sessionId %d!", m_NewSessionId);
		}

		ConnectionPtr connection = APP_CONTEXT.getConnectionStore()->getConnection(m_ConnectionId);

		if (connection) {
			connection->setAbout2SwitchSessionId(0);
		}
	}
}

