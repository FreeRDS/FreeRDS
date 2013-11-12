/**
 * Baseclass of an rpc call
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

#ifndef CALL_H_
#define CALL_H_

#include <string>
#include <stdint.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		class Call
		{
			public:
				Call();
				virtual ~Call();

				virtual unsigned long getCallType() = 0;
				virtual unsigned long getDerivedType() = 0;

				void setTag(uint32_t tag);
				uint32_t getTag();

				uint32_t getResult();
				std::string getErrorDescription();

			private:
				uint32_t mTag;
			protected:
				std::string mEncodedRequest;
				std::string mEncodedResponse;

				uint32_t mResult;
				// this is used if result ist not 0
				std::string mErrorDescription;
			};
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif
