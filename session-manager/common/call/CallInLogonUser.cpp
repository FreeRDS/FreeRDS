/**
 * Class for the LogonUser call.
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

#include "CallInLogonUser.h"
#include <appcontext/ApplicationContext.h>
#include <module/AuthModule.h>

using freerds::icp::LogonUserRequest;
using freerds::icp::LogonUserResponse;

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		static wLog* logger_CallInLogonUser = WLog_Get("freerds.SessionManager.call.callinlogonuser");


		CallInLogonUser::CallInLogonUser()
			: mConnectionId(0), mAuthStatus(0)
		{

		};

		CallInLogonUser::~CallInLogonUser()
		{

		};

		unsigned long CallInLogonUser::getCallType()
		{
			return freerds::icp::LogonUser;
		};

		int CallInLogonUser::decodeRequest()
		{
			// decode protocol buffers
			LogonUserRequest req;

			if (!req.ParseFromString(mEncodedRequest))
			{
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}

			mUserName = req.username();

			mConnectionId = req.connectionid();

			mDomainName = req.domain();

			mPassword = req.password();

			return 0;
		};

		int CallInLogonUser::encodeResponse()
		{
			// encode protocol buffers
			LogonUserResponse resp;
			// stup do stuff here

			//resp.set_authstatus(mAuthStatus);
			resp.set_serviceendpoint(mPipeName);

			if (!resp.SerializeToString(&mEncodedResponse))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}

			return 0;
		};

		int CallInLogonUser::authenticateUser() {

			sessionNS::ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getConnection(mConnectionId);
			if (currentConnection == NULL) {
				WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "Cannot get Connection for connectionId %lu",mConnectionId);
				mAuthStatus = -1;
				return -1;
			}
			mAuthStatus = currentConnection->authenticateUser(mUserName,mDomainName,mPassword);
			return mAuthStatus;
		}

		int CallInLogonUser::getUserSession() {


			sessionNS::ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getOrCreateConnection(mConnectionId);
			sessionNS::SessionPtr currentSession = APP_CONTEXT.getSessionStore()->getFirstSessionUserName(mUserName, mDomainName);

			if ((!currentSession) || (currentSession->getConnectState() != WTSDisconnected))
			{
				// create new Session for this request
				currentSession = APP_CONTEXT.getSessionStore()->createSession();
				currentSession->setUserName(mUserName);
				currentSession->setDomain(mDomainName);

				if (!currentSession->generateUserToken())
				{
					WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "generateUserToken failed for user %s with domain %s",mUserName.c_str(),mDomainName.c_str());
					mResult = 1;// will report error with answer
					return 1;
				}

				if (!currentSession->generateEnvBlockAndModify())
				{
					WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "generateEnvBlockAndModify failed for user %s with domain %s",mUserName.c_str(),mDomainName.c_str());
					mResult = 1;// will report error with answer
					return 1;
				}
				std::string moduleName;

				if (!APP_CONTEXT.getPropertyManager()->getPropertyString(currentSession->getSessionID(),"module",moduleName)) {
					moduleName = "X11";
				}
				currentSession->setModuleName(moduleName);
			}

			if (currentSession->getConnectState() == WTSDown)
			{
				std::string pipeName;
				if (!currentSession->startModule(pipeName))
				{
					WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "Module %s does not start properly for user %s in domain %s",currentSession->getModuleName().c_str(),mUserName.c_str(),mDomainName.c_str());
					mResult = 1;// will report error with answer
					return 1;
				}
			}

			currentConnection->setSessionId(currentSession->getSessionID());
			mPipeName = currentSession->getPipeName();
			return 0;
		}

		int CallInLogonUser::getAuthSession() {
			// authentication failed, start up greeter module
			sessionNS::ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getOrCreateConnection(mConnectionId);
			sessionNS::SessionPtr currentSession = APP_CONTEXT.getSessionStore()->createSession();

			std::string greeter;

			if (!APP_CONTEXT.getPropertyManager()->getPropertyString(0,"auth.greeter",greeter,mUserName)) {
				greeter = "Qt";
			}
			currentSession->setModuleName(greeter);

			if (!currentSession->startModule(greeter))
			{
				mResult = 1;// will report error with answer
				return 1;
			}

			currentConnection->setSessionId(currentSession->getSessionID());
			currentSession->setAuthSession(true);
			mPipeName = currentSession->getPipeName();
			return 0;
		}

		int CallInLogonUser::doStuff()
		{

			if (authenticateUser() != 0) {
				return 1;
			}
			if (mAuthStatus != 0) {
				return getAuthSession();
			} else {
				return getUserSession();
			}
		}

		}
	}
}
