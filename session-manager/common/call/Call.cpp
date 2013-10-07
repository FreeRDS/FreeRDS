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


#include "Call.h"
#include <string>
#include <winpr/handle.h>

namespace freerds{
	namespace sessionmanager{
		namespace call{

		Call::Call():mTag(0),mResult(0) {

		};

		Call::~Call() {
		};

		void Call::setTag(uint32_t tag) {
			mTag = 0;
		}

		uint32_t Call::getTag() {
			return mTag;
		}

		uint32_t Call::getResult() {
			return mResult;
		}

		std::string Call::getErrorDescription() {
			return mErrorDescription;
		}

		}
	}
}

