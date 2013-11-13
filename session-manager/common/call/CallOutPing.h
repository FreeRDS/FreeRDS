/**
 * Class for rpc call Ping (session manager to)
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

#ifndef __CALL_OUT_PING_H_
#define __CALL_OUT_PING_H_

#include <string>
#include "CallOut.h"
#include <ICP.pb.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{
			class CallOutPing: public CallOut
			{
			public:
				CallOutPing();
				virtual ~CallOutPing();

				virtual unsigned long getCallType();

				virtual int encodeRequest();
				virtual int decodeResponse();

				bool getPong();

			private:
				bool mPong;

			};
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif // __CALL_OUT_PING_H_
