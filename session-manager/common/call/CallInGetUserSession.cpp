/**
 * Class for rpc call GetUserSession (freerds to session manager)
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

#include "CallInGetUserSession.h"
#include <appcontext/ApplicationContext.h>

using freerds::icp::GetUserSessionRequest;
using freerds::icp::GetUserSessionResponse;

namespace freerds{
	namespace sessionmanager{
		namespace call{

		CallInGetUserSession::CallInGetUserSession() {

		};

		CallInGetUserSession::~CallInGetUserSession() {

		};

		unsigned long CallInGetUserSession::getCallType() {
			return freerds::icp::GetUserSession;
		};

		int CallInGetUserSession::decodeRequest() {
			// decode protocol buffers
			GetUserSessionRequest req;
			if (!req.ParseFromString(mEncodedRequest)) {
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}
			mUserName = req.username();
			if (req.has_domainname()) {
				mDomainName = req.domainname();
			}
			return 0;
		};

		int CallInGetUserSession::encodeResponse() {
			// encode protocol buffers
			GetUserSessionResponse resp;
			// stup do stuff here

			resp.set_sessionid(mSessionID);
			resp.set_serviceendpoint(mPipeName);

			if (!resp.SerializeToString(&mEncodedResponse)) {
				// failed to serialize
				mResult = 1;
				return -1;
			}
			return 0;
		};

		int CallInGetUserSession::doStuff() {
			std::string pipeName;

			sessionNS::Session * currentSession = APP_CONTEXT.getSessionStore()->getFirstSessionUserName(mUserName,mDomainName);
			if ((currentSession == NULL) || (currentSession->getConnectState() != WTSDisconnected) ) {
				currentSession = APP_CONTEXT.getSessionStore()->createSession();
				currentSession->setUserName(mUserName);
				currentSession->setDomain(mDomainName);

				if (!currentSession->generateUserToken()) {
					mResult = 1;// will report error with answer
					return 1;
				}

				if (!currentSession->generateEnvBlockAndModify() ){
					mResult = 1;// will report error with answer
					return 1;
				}

				currentSession->setModuleName("x11module");
			}


			if (currentSession->getConnectState() == WTSDown) {
				if (!currentSession->startModule(pipeName)) {
					mResult = 1;// will report error with answer
					return 1;
				}
			}
			mSessionID = currentSession->getSessionID();
			mPipeName = currentSession->getPipeName();

			return 0;
		}


		}
	}
}
