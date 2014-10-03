/**
 * Task for Session Timout callback.
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

#include "TaskSessionTimeout.h"
#include <call/TaskEndSession.h>
#include <appcontext/ApplicationContext.h>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace freerds
{
	namespace session
	{
		static wLog* logger_TaskSessionTimeout = WLog_Get("freerds.SessionManager.session.tasksessiontimeout");

		bool TaskSessionTimeout::isThreaded() {
			return true;
		};

		void TaskSessionTimeout::run()
		{
			long timeout;
			DWORD status;
			boost::posix_time::ptime myEpoch(boost::gregorian::date(1970,boost::gregorian::Jan, 1));

			for (;;)
			{
				status = WaitForSingleObject(mhStop,10 * 1000);

				if (status == WAIT_OBJECT_0) {
					// shutdown
					return;
				}

				if (status == WAIT_TIMEOUT)
				{
					// check all session if they need to be disconnected.
					std::list<SessionPtr> allSessions = APP_CONTEXT.getSessionStore()->getAllSessions();
					boost::posix_time::ptime currentTime = boost::date_time::second_clock<boost::posix_time::ptime>::universal_time();
					std::list<sessionNS::SessionPtr>::iterator iterator;

					for (iterator = allSessions.begin(); iterator != allSessions.end(); ++iterator)
					{
						sessionNS::SessionPtr currentSession = (*iterator);

						if (currentSession->getConnectState() == WTSDisconnected)
						{
							//currentSession->getUserName()
							if (!APP_CONTEXT.getPropertyManager()->getPropertyNumber("session.timeout", &timeout)) {
								WLog_Print(logger_TaskSessionTimeout, WLOG_INFO, "session.timeout was not found for session %d, using value of 0", currentSession->getSessionID());
								timeout = 0;
							}

							if ((timeout >= 0) &&(((currentTime - currentSession->getConnectStateChangeTime()).ticks()) / ((boost::posix_time::time_duration::ticks_per_second() * 60)) >= timeout)) {
								// shutdown current Session
								WLog_Print(logger_TaskSessionTimeout, WLOG_INFO, "Session with sessionId %d from user %s is stopped after %d minutes after the disconnect. ",currentSession->getSessionID(),currentSession->getUserName().c_str(),timeout);
								callNS::TaskEndSessionPtr task = callNS::TaskEndSessionPtr(new callNS::TaskEndSession());
								task->setSessionId(currentSession->getSessionID());
								APP_CONTEXT.addTask(task);
							}
						}
					}
				}
			}
		}
	}
}

