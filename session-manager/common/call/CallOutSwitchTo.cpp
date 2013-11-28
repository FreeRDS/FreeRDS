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

		bool CallOutSwitchTo::isSuccess() {
			return mSuccess;
		}


		}
	}
}

