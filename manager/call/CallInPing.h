/**
 * Class for rpc call Ping (freerds to session manager)
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

#ifndef _CALL_IN_PING_H_
#define _CALL_IN_PING_H_

#include "CallFactory.h"
#include <string>
#include "CallIn.h"
#include <ICP.pb.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{
			class CallInPing: public CallIn
			{
			public:
				CallInPing();
				virtual ~CallInPing();

				virtual unsigned long getCallType();
				virtual int decodeRequest();
				virtual int encodeResponse();
				virtual int doStuff();

			private:

			};

			FACTORY_REGISTER_DWORD(CallFactory,CallInPing,freerds::icp::Ping);
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif // _CALL_IN_PING_H_
