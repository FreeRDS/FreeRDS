/**
 * Connection class
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

#include <winpr/crt.h>
#include <winpr/wlog.h>
#include <winpr/pipe.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/environment.h>

#include <iostream>
#include <sstream>

#include <appcontext/ApplicationContext.h>

#include "Connection.h"

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{

			static wLog* logger_Connection = WLog_Get("freerds.sessionmanager.session.connection");

			Connection::Connection(DWORD connectionId)
				: mConnectionId(connectionId),mSessionId(0)
			{
			}

			Connection::~Connection()
			{

			}

			std::string Connection::getDomain() {
				return mDomain;
			}

			void Connection::setDomain(std::string domainName) {
				mDomain = domainName;
			}

			std::string Connection::getUserName() {
				return mUsername;
			}
			void Connection::setUserName(std::string username) {
				mUsername = username;
			}

			void Connection::setSessionId(long sessionId) {
				mSessionId = sessionId;
			}

			long Connection::getSessionId() {
				return mSessionId;
			}

			long Connection::getConnectionId() {
				return mConnectionId;
			}


		}
	}
}

