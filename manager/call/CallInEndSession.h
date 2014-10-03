/**
 * Class for rpc call EndSession (service to session manager)
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

#ifndef CALL_IN_END_SESSION_H_
#define CALL_IN_END_SESSION_H_

#include <string>

#include "CallFactory.h"
#include "CallIn.h"

#include <ICPS.pb.h>

namespace freerds
{
	namespace call
	{
		class CallInEndSession: public CallIn
		{
		public:
			CallInEndSession();
			virtual ~CallInEndSession();

			virtual unsigned long getCallType();
			virtual int decodeRequest();
			virtual int encodeResponse();
			virtual int doStuff();

		private:
			long mSessionId;
			bool mSuccess;
		};

		FACTORY_REGISTER_DWORD(CallFactory,CallInEndSession,freerds::icps::EndSession);
	}
}

namespace callNS = freerds::call;

#endif
