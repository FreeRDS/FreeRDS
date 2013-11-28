 /**
 * Class for rpc call SwitchTo (session manager to freerds)
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

#include "CallOutSwitchTo.h"
#include <appcontext/ApplicationContext.h>

using freerds::icp::SwitchToRequest;
using freerds::icp::SwitchToResponse;

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		static wLog * logger_CallOutSwitchTo = WLog_Get("freerds.sessionmanager.call.calloutswitchto");

		CallOutSwitchTo::CallOutSwitchTo()
		{
			mConnectionId = 0;
			mSuccess = false;
			mOldSessionId = 0;
			mNewSessionId = 0;
		};

		CallOutSwitchTo::~CallOutSwitchTo()
		{

		};

		unsigned long CallOutSwitchTo::getCallType()
		{
			return freerds::icp::SwitchTo;
		};


		int CallOutSwitchTo::encodeRequest(){
			SwitchToRequest req;
			req.set_connectionid(mConnectionId);
			req.set_serviceendpoint(mServiceEndpoint);

			if (!req.SerializeToString(&mEncodedRequest))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}
			return 0;
		}

		int CallOutSwitchTo::decodeResponse() {
			SwitchToResponse resp;

			if (!resp.ParseFromString(mEncodedResponse))
			{
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}
			mSuccess = resp.success();
			return 0;
		}


		void CallOutSwitchTo::setConnectionId(long connectionId) {
			mConnectionId = connectionId;
		}

		void CallOutSwitchTo::setServiceEndpoint(std::string serviceEndpoint) {
			mServiceEndpoint = serviceEndpoint;
		}

		bool CallOutSwitchTo::hasAnswerCallback() {
			return true;
		}

		void CallOutSwitchTo::answerCallback() {

			if (mResult != 0) {
				WLog_Print(logger_CallOutSwitchTo, WLOG_ERROR, "CallOutSwitchTo answercallback: RPC error %d!",mResult);
				return;
			}
			// first unpack the answer
			if (decodeResponse()) {
				//
				WLog_Print(logger_CallOutSwitchTo, WLOG_ERROR, "CallOutSwitchTo answercallback: decoding failed!");
				return;
			}
			if (!mSuccess) {
				WLog_Print(logger_CallOutSwitchTo, WLOG_ERROR, "CallOutSwitchTo answercallback: switching in FreeRDS failed!");
				return;
			}
			sessionNS::SessionPtr currentSession;

			if (mOldSessionId != 0) {
				currentSession = APP_CONTEXT.getSessionStore()->getSession(mOldSessionId);
				if (!currentSession) {
					currentSession->stopModule();
					APP_CONTEXT.getSessionStore()->removeSession(currentSession->getSessionID());
					WLog_Print(logger_CallOutSwitchTo, WLOG_INFO, "CallOutSwitchTo answercallback: session with sessionId %d was stopped!",mOldSessionId);
				} else {
					WLog_Print(logger_CallOutSwitchTo, WLOG_ERROR, "CallOutSwitchTo answercallback: no session was found for sessionId %d!",mOldSessionId);
				}
			} else {
				WLog_Print(logger_CallOutSwitchTo, WLOG_ERROR, "CallOutSwitchTo answercallback: no oldSessionId was set!");
			}

			APP_CONTEXT.getConnectionStore()->getOrCreateConnection(mConnectionId)->setSessionId(mNewSessionId);
			return;
		}

		void CallOutSwitchTo::setOldSessionId(long sessionId) {
			mOldSessionId = sessionId;
		}

		void CallOutSwitchTo::setNewSessionId(long sessionId) {
			mNewSessionId = sessionId;
		}

		}
	}
}


