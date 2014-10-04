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

#ifndef CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_
#define CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_

#include <string>
#include "CallOut.h"
#include <ICP.pb.h>

namespace freerds
{
	namespace call
	{
		class CallOutFdsApiVirtualChannelOpen: public CallOut
		{
		public:
			CallOutFdsApiVirtualChannelOpen();
			virtual ~CallOutFdsApiVirtualChannelOpen();

			virtual unsigned long getCallType();

			virtual int encodeRequest();
			virtual int decodeResponse();

			void setSessionID(long sessionID);
			void setVirtualName(std::string virtualName);
			std::string getConnectionString();

		private:
			long mSessionID;
			std::string mVirtualName;
			std::string mConnectionString;

			FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST request;
			FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE response;
		};
	}
}

namespace callNS = freerds::call;

#endif //CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_
