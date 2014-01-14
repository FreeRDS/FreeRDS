/**
 * Class for rpc call DisconnectUserSession (freerds to session manager)
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

#include "CallInDisconnectUserSession.h"
#include <appcontext/ApplicationContext.h>
#include <call/TaskEndSession.h>

using freerds::icp::DisconnectUserSessionRequest;
using freerds::icp::DisconnectUserSessionResponse;

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		CallInDisconnectUserSession::CallInDisconnectUserSession()
		{
			mConnectionId = 0;
			mDisconnected = false;
		};

		CallInDisconnectUserSession::~CallInDisconnectUserSession()
		{

		};

		unsigned long CallInDisconnectUserSession::getCallType()
		{
			return freerds::icp::DisconnectUserSession;
		};

		int CallInDisconnectUserSession::decodeRequest()
		{
			// decode protocol buffers
			DisconnectUserSessionRequest req;
			if (!req.ParseFromString(mEncodedRequest))
			{
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}
			mConnectionId = req.connectionid();
			return 0;
		};

		int CallInDisconnectUserSession::encodeResponse()
		{
			// encode protocol buffers
			DisconnectUserSessionResponse resp;
			// stup do stuff here

			resp.set_disconnected(mDisconnected);

			if (!resp.SerializeToString(&mEncodedResponse))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}

			return 0;
		};

		int CallInDisconnectUserSession::doStuff()
		{
			sessionNS::ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getConnection(mConnectionId);
			if ((currentConnection == NULL) || (currentConnection->getSessionId() == 0)) {
				mDisconnected = false;
				return -1;
			}

			sessionNS::SessionPtr currentSession = APP_CONTEXT.getSessionStore()->getSession(currentConnection->getSessionId());

			if (!currentSession)
			{
				mDisconnected = false;
				return -1;
			}

			currentSession->setConnectState(WTSDisconnected);
			APP_CONTEXT.getConnectionStore()->removeConnection(mConnectionId);

			long timeout;

			if (!APP_CONTEXT.getPropertyManager()->getPropertyNumber(currentSession->getSessionID(),"session.timeout",timeout)) {
				timeout = 0;
			}

			 bool reconnectAllowd;
             if (!APP_CONTEXT.getPropertyManager()->getPropertyBool(currentSession->getSessionID(),"session.reconnect",reconnectAllowd)) {
            	 reconnectAllowd = true;
             }

			if ((timeout == 0) && (!reconnectAllowd)) {
				callNS::TaskEndSessionPtr task = callNS::TaskEndSessionPtr(new callNS::TaskEndSession());
				task->setSessionId(currentSession->getSessionID());
				APP_CONTEXT.addTask(task);
			}

			mDisconnected = true;
			return 0;
		}
		}
	}
}
