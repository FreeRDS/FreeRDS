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

#include "CallInLogonUser.h"
#include <appcontext/ApplicationContext.h>

using freerds::icp::LogonUserRequest;
using freerds::icp::LogonUserResponse;

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{
		CallInLogonUser::CallInLogonUser()
			: mSessionId(0)
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

			//mSessionId = req.sessionid();
			mUserName = req.username();

			if (req.has_domain())
				mDomainName = req.domain();

			if (req.has_password())
				mPassword = req.password();

			return 0;
		};

		int CallInLogonUser::encodeResponse()
		{
			// encode protocol buffers
			LogonUserResponse resp;
			// stup do stuff here

			resp.set_sessionid(mSessionId);
			resp.set_authstatus(mAuthStatus);
			resp.set_serviceendpoint(mPipeName);

			if (!resp.SerializeToString(&mEncodedResponse))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}

			return 0;
		};

		int CallInLogonUser::doStuff()
		{
			std::string pipeName;

			sessionNS::Session* currentSession = APP_CONTEXT.getSessionStore()->getFirstSessionUserName(mUserName, mDomainName);

			if ((!currentSession) || (currentSession->getConnectState() != WTSDisconnected))
			{
				currentSession = APP_CONTEXT.getSessionStore()->createSession();
				currentSession->setUserName(mUserName);
				currentSession->setDomain(mDomainName);

				if (!currentSession->generateUserToken())
				{
					mResult = 1;// will report error with answer
					return 1;
				}

				if (!currentSession->generateEnvBlockAndModify())
				{
					mResult = 1;// will report error with answer
					return 1;
				}

				char* moduleName = APP_CONTEXT.getModuleManager()->getDefaultModuleName();
				currentSession->setModuleName(moduleName);
			}

			if (currentSession->getConnectState() == WTSDown)
			{
				if (!currentSession->startModule(pipeName))
				{
					mResult = 1;// will report error with answer
					return 1;
				}
			}

			mSessionId = currentSession->getSessionID();

			mPipeName = currentSession->getPipeName();
			mAuthStatus = 0;

			return 0;
		}
		}
	}
}
