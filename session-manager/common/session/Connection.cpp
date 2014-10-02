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

#include <appcontext/ApplicationContext.h>
#include <module/AuthModule.h>
#include <utils/CSGuard.h>


#include "Connection.h"

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{
			static wLog* logger_Connection = WLog_Get("freerds.sessionmanager.session.connection");

			Connection::Connection(DWORD connectionId)
				: mConnectionId(connectionId),mSessionId(0),mAuthStatus(-1),mAbout2SwitchSessionId(0)
			{
				if (!InitializeCriticalSectionAndSpinCount(&mCSection, 0x00000400))
				{
					 WLog_Print(logger_Connection, WLOG_FATAL, "cannot init SessionStore critical section!");
				}

				ZeroMemory(&mClientInformation, sizeof(mClientInformation));
			}

			Connection::~Connection()
			{

			}

			std::string Connection::getDomain() {
				return mDomain;
			}

			std::string Connection::getUserName() {
				return mUsername;
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

			int Connection::authenticateUser(std::string username, std::string domain, std::string password)
			{
				CSGuard guard(&mCSection);

				if (mAuthStatus == 0) {
					// a Connection can only be authorized once
					return -1;
				}

				std::string authModule;

				if (!APP_CONTEXT.getPropertyManager()->getPropertyString("auth.module", authModule)) {
					authModule = "PAM";
				}

				moduleNS::AuthModule* auth = moduleNS::AuthModule::loadFromName(authModule);

				if (!auth) {
					return 1;
				}

				mAuthStatus = auth->logonUser(username, domain, password);

				delete auth;

				if (mAuthStatus == 0) {
					mUsername = username;
					mDomain = domain;
				}

				return mAuthStatus;

			}

			pCLIENT_INFORMATION Connection::getClientInformation() {
				return &mClientInformation;
			}

			bool Connection::getProperty(std::string path, PROPERTY_STORE_HELPER& helper)
			{
				if (path.compare("xres") == 0) {
					helper.type = NumberType;
					helper.numberValue = mClientInformation.with;
					return true;
				} else if (path.compare("yres") == 0) {
					helper.type = NumberType;
					helper.numberValue = mClientInformation.height;
					return true;
				} else if (path.compare("colordepth") == 0) {
					helper.type = NumberType;
					helper.numberValue = mClientInformation.colordepth;
					return true;
				}
				return false;
			}

			long Connection::getAbout2SwitchSessionId() {
				return mAbout2SwitchSessionId;
			}

			void Connection::setAbout2SwitchSessionId(long switchSessionId) {
				mAbout2SwitchSessionId = switchSessionId;
			}
		}
	}
}

