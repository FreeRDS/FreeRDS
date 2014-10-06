/**
 * Class for authenticating a user. This call comes not from the
 * 		FreeRDS, but it comes from the service instead and is only
 * 		tunneled at FreeRDS.
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

#ifndef __CALL_IN_AUTHENTICATE_USER_H_
#define __CALL_IN_AUTHENTICATE_USER_H_

#include <winpr/crt.h>

#include <string>

#include "CallFactory.h"
#include "CallIn.h"

namespace freerds
{
	namespace call
	{
		class CallInAuthenticateUser: public CallIn
		{
		public:
			CallInAuthenticateUser();
			virtual ~CallInAuthenticateUser();

			virtual unsigned long getCallType();
			virtual int decodeRequest();
			virtual int encodeResponse();
			virtual int doStuff();

		private:
			int authenticateUser();
			int getAuthSession();
			int getUserSession();

			std::string mUserName;
			std::string mDomainName;
			std::string mPassword;

			int mAuthStatus;
			UINT32 mSessionId;
			std::string mPipeName;

			UINT32 m_RequestId;
			FDSAPI_START_SESSION_REQUEST m_Request;

			UINT32 m_ResponseId;
			FDSAPI_START_SESSION_RESPONSE m_Response;
		};

		FACTORY_REGISTER_DWORD(CallFactory, CallInAuthenticateUser, FDSAPI_START_SESSION_REQUEST_ID);
	}
}

namespace callNS = freerds::call;

#endif// __CALL_IN_AUTHENTICATE_USER_H_
