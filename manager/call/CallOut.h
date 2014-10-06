 /**
 * Baseclass for outgoing rpc calls (session manager to freerds)
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

#ifndef CALLOUT_H_
#define CALLOUT_H_

#include <call/Call.h>
#include <winpr/synch.h>

namespace freerds
{
	namespace call
	{
		class CallOut:public Call
		{
		public:
			CallOut();
			~CallOut();

			virtual unsigned long getDerivedType();

			virtual int encodeRequest() = 0;
			std::string getEncodedRequest();

			void setEncodedeResponse(std::string encodedResponse);
			virtual int decodeResponse() = 0;

			void   initAnswerHandle();
			HANDLE getAnswerHandle();

			void setResult(uint32_t result);
			void setErrorDescription(std::string error);

		private :
			HANDLE mAnswer;
		};
	}
}

namespace callNS = freerds::call;


#endif /* CALLOUT_H_ */
