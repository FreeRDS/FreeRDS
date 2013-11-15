/**
 *  FDSApiHandler
 *
 *  Starts and stops the thrift server.
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
#include "FDSApiHandler.h"
#include <appcontext/ApplicationContext.h>

#include <winpr/thread.h>
#include <winpr/synch.h>
#include <winpr/wlog.h>

#include <call/CallOutFdsApiVirtualChannelOpen.h>


namespace freerds{
	namespace sessionmanager{
		namespace fdsapi {

		static wLog * logger_FDSApiHandler = WLog_Get("freerds.sessionmanager.fdsapihandler");


		FDSApiHandler::FDSApiHandler()
		{
		}

		FDSApiHandler::~FDSApiHandler()
		{
		}

		TDWORD FDSApiHandler::ping(const TDWORD input)
		{
		}

		void FDSApiHandler::virtualChannelOpen(TLPSTR& _return,
				const TLPSTR& authToken, const TDWORD sessionId,
				const TLPSTR& virtualName)
		{
			// ....

			callNS::CallOutFdsApiVirtualChannelOpen openCall;
			openCall.setSessionID(sessionId);
			openCall.setVirtualName(virtualName);

			APP_CONTEXT.getRpcOutgoingQueue()->addElement(&openCall);
			WaitForSingleObject(openCall.getAnswerHandle(),INFINITE);
			if (openCall.getResult() == 0) {
				// no error
				_return = openCall.getConnectionString();
			} else {
				// report error
				_return = "";
			}

		}

		void FDSApiHandler::virtualChannelOpenEx(
				TLPSTR& _return, const TLPSTR& authToken, const TDWORD sessionId,
				const TLPSTR& virtualName, const TDWORD flags)
		{
		}

		bool FDSApiHandler::virtualChannelClose(
				const TLPSTR& authToken, const TDWORD sessionId,
				const TLPSTR& virtualName)
		{
		}

		bool FDSApiHandler::disconnectSession(
				const TLPSTR& authToken, const TDWORD sessionId, const bool wait)
		{
		}

		bool FDSApiHandler::logoffSession(
				const TLPSTR& authToken, const TDWORD sessionId, const bool wait)
		{
		}

		void FDSApiHandler::enumerateSessions(
				ReturnEnumerateSession& _return, const TLPSTR& authToken,
				const TDWORD Version)
		{
		}

		}
	}
}


