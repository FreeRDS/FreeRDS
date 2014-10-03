/**
 * Class for rpc call LogOffUserSession (freerds to session manager)
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

#include "CallInLogOffUserSession.h"
#include <appcontext/ApplicationContext.h>

using freerds::icp::LogOffUserSessionRequest;
using freerds::icp::LogOffUserSessionResponse;

namespace freerds
{
	namespace call
	{
		CallInLogOffUserSession::CallInLogOffUserSession()
		{
			mConnectionId = 0;
			mLoggedOff = false;
		};

		CallInLogOffUserSession::~CallInLogOffUserSession()
		{

		};

		unsigned long CallInLogOffUserSession::getCallType()
		{
			return freerds::icp::LogOffUserSession;
		};

		int CallInLogOffUserSession::decodeRequest()
		{
			// decode protocol buffers
			LogOffUserSessionRequest req;

			if (!req.ParseFromString(mEncodedRequest))
			{
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}

			mConnectionId = req.connectionid();

			return 0;
		};

		int CallInLogOffUserSession::encodeResponse()
		{
			// encode protocol buffers
			LogOffUserSessionResponse resp;
			// stup do stuff here

			resp.set_loggedoff(mLoggedOff);

			if (!resp.SerializeToString(&mEncodedResponse))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}

			return 0;
		};

		int CallInLogOffUserSession::doStuff()
		{

			sessionNS::ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getConnection(mConnectionId);
			if ((currentConnection == NULL) || (currentConnection->getSessionId() == 0)) {
				mLoggedOff = false;
				return -1;
			}
			sessionNS::SessionPtr currentSession = APP_CONTEXT.getSessionStore()->getSession(currentConnection->getSessionId());

			if (!currentSession) {
				mLoggedOff = false;
				return -1;
			}


			currentSession->stopModule();

			// TODO check if more than 1 connection is connected to this session

			APP_CONTEXT.getSessionStore()->removeSession(currentConnection->getSessionId());
			APP_CONTEXT.getConnectionStore()->removeConnection(mConnectionId);
			mLoggedOff = true;
			return 0;
		}
	}
}
