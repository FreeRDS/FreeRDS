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

#include "TaskSwitchTo.h"
#include <winpr/wlog.h>
#include <appcontext/ApplicationContext.h>
#include "CallOutSwitchTo.h"


namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{
			static wLog* logger_taskSwitchTo = WLog_Get("freerds.SessionManager.call.taskswitchto");

			void TaskSwitchTo::run() {

				CallOutSwitchTo switchToCall;
				switchToCall.setServiceEndpoint(mServiceEndpoint);
				switchToCall.setConnectionId(mConnectionId);

				APP_CONTEXT.getRpcOutgoingQueue()->addElement(&switchToCall);
				WaitForSingleObject(switchToCall.getAnswerHandle(),INFINITE);


				if (switchToCall.getResult() != 0) {
					WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo answer: RPC error %d!",switchToCall.getResult());
					return cleanUpOnError();
				}
				// first unpack the answer
				if (switchToCall.decodeResponse()) {
					//
					WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: decoding of switchto answer failed!");
					return cleanUpOnError();
				}
				if (!switchToCall.isSuccess()) {
					WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: switching in FreeRDS failed!");
					return cleanUpOnError();
				}
				sessionNS::SessionPtr currentSession;

				if (mOldSessionId != 0) {
					currentSession = APP_CONTEXT.getSessionStore()->getSession(mOldSessionId);
					if (currentSession != NULL) {
						currentSession->stopModule();
						APP_CONTEXT.getSessionStore()->removeSession(currentSession->getSessionID());
						WLog_Print(logger_taskSwitchTo, WLOG_INFO, "TaskSwitchTo: session with sessionId %d was stopped!",mOldSessionId);
					} else {
						WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: no session was found for sessionId %d!",mOldSessionId);
					}
				} else {
					WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: no oldSessionId was set!");
				}

				sessionNS::ConnectionPtr connection = APP_CONTEXT.getConnectionStore()->getConnection(mConnectionId);
				if (connection != NULL) {
					connection->setSessionId(mNewSessionId);
					connection->setAbout2SwitchSessionId(0);
				}

				return;
			}

			void TaskSwitchTo::setConnectionId(long connectionId) {
				mConnectionId = connectionId;
			}

			void TaskSwitchTo::setServiceEndpoint(std::string serviceEndpoint) {
				mServiceEndpoint = serviceEndpoint;
			}

			void TaskSwitchTo::setOldSessionId(long sessionId) {
				mOldSessionId = sessionId;
			}

			void TaskSwitchTo::setNewSessionId(long sessionId) {
				mNewSessionId = sessionId;
			}

			void TaskSwitchTo::cleanUpOnError() {
				sessionNS::SessionPtr currentSession = APP_CONTEXT.getSessionStore()->getSession(mNewSessionId);
				if (currentSession != NULL) {
					if (currentSession->getConnectState() == WTSActive) {
						// this was a new session for the connection, remove it
						currentSession->stopModule();
						APP_CONTEXT.getSessionStore()->removeSession(currentSession->getSessionID());
						WLog_Print(logger_taskSwitchTo, WLOG_INFO, "TaskSwitchTo: cleaning up session with sessionId %d",mNewSessionId);
					} else if (currentSession->getConnectState() == WTSConnectQuery){
						// was a previous disconnected session
						currentSession->setConnectState(WTSDisconnected);
					}
				} else {
					WLog_Print(logger_taskSwitchTo, WLOG_ERROR, "TaskSwitchTo: no session was found for sessionId %d!",mNewSessionId);
				}
				sessionNS::ConnectionPtr connection = APP_CONTEXT.getConnectionStore()->getConnection(mConnectionId);
				if (connection != NULL) {
					connection->setAbout2SwitchSessionId(0);
				}

			}

		}
	}
}

