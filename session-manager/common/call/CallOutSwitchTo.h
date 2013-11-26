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

#ifndef CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_
#define CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_

#include <string>
#include "CallOut.h"
#include <ICP.pb.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{
			class CallOutSwitchTo: public CallOut
			{
			public:
				CallOutSwitchTo();
				virtual ~CallOutSwitchTo();

				virtual unsigned long getCallType();

				virtual int encodeRequest();
				virtual int decodeResponse();

				void setConnectionId(long connectionId);
				void setServiceEndpoint(std::string serviceEndpoint);

				bool	hasAnswerCallback();
				virtual void answerCallback();

				// functions needed for callback
				void setOldSessionId(long sessionId);
				void setNewSessionId(long sessionId);

			private:
				long mConnectionId;
				std::string mServiceEndpoint;
				bool mSuccess;

				// variables needed for the callback
				long mOldSessionId;
				long mNewSessionId;
			};
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif //CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_
