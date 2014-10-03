/**
 * Class for ping rpc call (freerds to session manager)
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
 * Copyright 2013 Bernhard Miklautz <bernhard.miklautz@thincast.com>
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

#include "CallInPing.h"

using freerds::icp::PingRequest;
using freerds::icp::PingResponse;

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		CallInPing::CallInPing()
		{

		};

		CallInPing::~CallInPing()
		{

		};

		unsigned long CallInPing::getCallType()
		{
			return freerds::icp::Ping;
		};

		int CallInPing::decodeRequest()
		{
			// decode protocol buffers
			PingRequest req;

			if (!req.ParseFromString(mEncodedRequest))
			{
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}

			return 0;
		};

		int CallInPing::encodeResponse()
		{
			// encode protocol buffers
			PingResponse resp;
			resp.set_pong(true);

			if (!resp.SerializeToString(&mEncodedResponse))
			{
				// failed to serialize
				mResult = 1;
				return -1;
			}
			return 0;
		};

		int CallInPing::doStuff()
		{
			return 0;
		}
		}
	}
}
