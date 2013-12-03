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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CallOut.h"
#include <string>
#include <winpr/handle.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{
		CallOut::CallOut():mAnswer(NULL)
		{
			initAnswerHandle();
		};

		CallOut::~CallOut()
		{
			if (mAnswer)
			{
				CloseHandle(mAnswer);
				mAnswer = NULL;
			}

		};

		std::string CallOut::getEncodedRequest()
		{
			return mEncodedRequest;
		}

		void CallOut::setEncodedeResponse(std::string encodedResponse)
		{
			mEncodedResponse = encodedResponse;
		}

		void CallOut::initAnswerHandle()
		{
			if (mAnswer == NULL) {
				mAnswer = CreateEvent(NULL,TRUE,FALSE,NULL);
			}
		}

		HANDLE CallOut::getAnswerHandle()
		{
			return mAnswer;
		}

		unsigned long CallOut::getDerivedType()
		{
			return 2; // for all CallIns
		}

		void CallOut::setResult(uint32_t result)
		{
			mResult = result;
			SetEvent(mAnswer);
		}

		void CallOut::setErrorDescription(std::string error)
		{
			mErrorDescription = error;
		}

		}
	}
}

