/**
 * Class for rpc call LogOffUserSession (session manager to freerds)
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

#include "CallOutLogOffUserSession.h"
#include <appcontext/ApplicationContext.h>

using freerds::icp::LogOffUserSessionRequest;
using freerds::icp::LogOffUserSessionResponse;

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{
		CallOutLogOffUserSession::CallOutLogOffUserSession()
		{
			mConnectionId = 0;
			mLoggedOff = false;
		};

		CallOutLogOffUserSession::~CallOutLogOffUserSession()
		{

		};

		unsigned long CallOutLogOffUserSession::getCallType()
		{
			return freerds::icp::LogOffUserSession;
		};

		int CallOutLogOffUserSession::encodeRequest()
		{
			// decode protocol buffers
			LogOffUserSessionRequest req;
			req.set_connectionid(mConnectionId);

			if (!req.SerializeToString(&mEncodedRequest))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}

			return 0;
		};

		int CallOutLogOffUserSession::decodeResponse()
		{
			// encode protocol buffers
			LogOffUserSessionResponse resp;
			// stup do stuff here

			if (!resp.ParseFromString(mEncodedResponse))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}

			mLoggedOff = resp.loggedoff();

			return 0;
		};

		void CallOutLogOffUserSession::setConnectionId(long connectionId) {
			mConnectionId = connectionId;
		}

		bool CallOutLogOffUserSession::isLoggedOff() {
			return mLoggedOff;
		}

}
	}
}
