/**
 * Class for rpc call CallInEndSession (service to session manager)
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

#include "CallInEndSession.h"
#include "TaskEndSession.h"
#include <appcontext/ApplicationContext.h>

using freerds::icps::EndSessionRequest;
using freerds::icps::EndSessionResponse;

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		CallInEndSession::CallInEndSession()
		{
			mSessionId = 0;
			mSuccess = false;
		};

		CallInEndSession::~CallInEndSession()
		{

		};

		unsigned long CallInEndSession::getCallType()
		{
			return freerds::icps::EndSession;
		};

		int CallInEndSession::decodeRequest()
		{
			// decode protocol buffers
			EndSessionRequest req;
			if (!req.ParseFromString(mEncodedRequest))
			{
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}
			mSessionId = req.sessionid();
			return 0;
		};

		int CallInEndSession::encodeResponse()
		{
			// encode protocol buffers
			EndSessionResponse resp;

			resp.set_success(mSuccess);

			if (!resp.SerializeToString(&mEncodedResponse))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}

			return 0;
		};

		int CallInEndSession::doStuff()
		{
			callNS::TaskEndSessionPtr task = callNS::TaskEndSessionPtr(new callNS::TaskEndSession());
			task->setSessionId(mSessionId);
			APP_CONTEXT.addTask(task);
			mSuccess = true;
			return 0;
		}
		}
	}
}
