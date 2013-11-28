/**
 * Class for rpc call LogonUser (freerds to session manager)
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

#include "CallInAuthenticateUser.h"
#include "CallOutSwitchTo.h"
#include <appcontext/ApplicationContext.h>
#include <module/AuthModule.h>

using freerds::icps::AuthenticateUserRequest;
using freerds::icps::AuthenticateUserResponse;

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		static wLog* logger_CallInLogonUser = WLog_Get("freerds.SessionManager.call.callinauthenticateuser");


		CallInAuthenticateUser::CallInAuthenticateUser()
			: mSessionId(0), mAuthStatus(0)
		{

		};

		CallInAuthenticateUser::~CallInAuthenticateUser()
		{

		};

		unsigned long CallInAuthenticateUser::getCallType()
		{
			return freerds::icps::AuthenticateUser;
		};

		int CallInAuthenticateUser::decodeRequest()
		{
			// decode protocol buffers
			AuthenticateUserRequest req;

			if (!req.ParseFromString(mEncodedRequest))
			{
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}

			mUserName = req.username();

			mSessionId = req.sessionid();

			mDomainName = req.domain();

			mPassword = req.password();

			return 0;
		};

		int CallInAuthenticateUser::encodeResponse()
		{
			// encode protocol buffers
			AuthenticateUserResponse resp;
			// stup do stuff here

			if (mAuthStatus == 0) {
				resp.set_authstatus(freerds::icps::AuthenticateUserResponse_AUTH_STATUS_AUTH_SUCCESSFULL);
			} else {
				resp.set_authstatus(freerds::icps::AuthenticateUserResponse_AUTH_STATUS_AUTH_BAD_CREDENTIAL);
			}

			if (!resp.SerializeToString(&mEncodedResponse))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}

			return 0;
		};

		int CallInAuthenticateUser::authenticateUser() {

			std::string authModule;
			if (!APP_CONTEXT.getPropertyManager()->getPropertyString(0,"auth.module",authModule,mUserName)) {
				authModule = "PAM";
			}

			moduleNS::AuthModule* auth = moduleNS::AuthModule::loadFromName(authModule);

			if (!auth) {
				mResult = 1;
				return 1;
			}

			mAuthStatus = auth->logonUser(mUserName, mDomainName, mPassword);

			delete auth;
			return 0;

		}

		int CallInAuthenticateUser::getUserSession() {

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

			CallOutSwitchTo * switchToCall = new CallOutSwitchTo();
			switchToCall->setServiceEndpoint(currentSession->getPipeName());
			switchToCall->setOldSessionId(mSessionId);
			switchToCall->setNewSessionId(currentSession->getSessionID());
			switchToCall->setConnectionId(APP_CONTEXT.getConnectionStore()->getConnectionIdForSessionId(mSessionId));

			APP_CONTEXT.getRpcOutgoingQueue()->addElement(switchToCall);

			return 0;
		}



		int CallInAuthenticateUser::doStuff()
		{
			if (authenticateUser() == 0) {
				if (mAuthStatus == 0) {
					// user is authenticated
					return getUserSession();
				}
			}
			return 1;
		}

		}
	}
}
