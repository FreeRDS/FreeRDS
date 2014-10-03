/**
 /**
 * Class for rpc call FdsApiVirtualChannelOpen (session manager to freerds)
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

#include "CallOutFdsApiVirtualChannelOpen.h"
#include <appcontext/ApplicationContext.h>

using freerds::icp::FdsApiVirtualChannelOpenRequest;
using freerds::icp::FdsApiVirtualChannelOpenResponse;

namespace freerds
{
	namespace call
	{
		CallOutFdsApiVirtualChannelOpen::CallOutFdsApiVirtualChannelOpen()
		{
			mSessionID = 0;
		};

		CallOutFdsApiVirtualChannelOpen::~CallOutFdsApiVirtualChannelOpen()
		{

		};

		unsigned long CallOutFdsApiVirtualChannelOpen::getCallType()
		{
			return freerds::icp::FdsApiVirtualChannelOpen;
		};


		int CallOutFdsApiVirtualChannelOpen::encodeRequest()
		{
			FdsApiVirtualChannelOpenRequest req;
			req.set_sessionid(mSessionID);
			req.set_virtualname(mVirtualName);

			if (!req.SerializeToString(&mEncodedRequest))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}
			return 0;
		}

		int CallOutFdsApiVirtualChannelOpen::decodeResponse()
		{
			FdsApiVirtualChannelOpenResponse resp;

			if (!resp.ParseFromString(mEncodedResponse))
			{
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}
			mConnectionString = resp.connectionstring();
			return 0;
		}


		void CallOutFdsApiVirtualChannelOpen::setSessionID(long sessionID) {
			mSessionID = sessionID;
		}

		void CallOutFdsApiVirtualChannelOpen::setVirtualName(std::string virtualName) {
			mVirtualName = virtualName;
		}

		std::string CallOutFdsApiVirtualChannelOpen::getConnectionString() {
			return mConnectionString;
		}
	}
}
