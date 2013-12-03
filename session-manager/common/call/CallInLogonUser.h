/**
 * Class for the LogonUser call.
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

#ifndef __CALL_IN_LOGON_USER_H_
#define __CALL_IN_LOGON_USER_H_

#include <winpr/crt.h>

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
			class CallInLogonUser: public CallIn
			{
			public:
				CallInLogonUser();
				virtual ~CallInLogonUser();

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

				long mWidth;
				long mHeight;
				long mColorDepth;

				int mAuthStatus;
				UINT32 mConnectionId;
				std::string mPipeName;
			};

			FACTORY_REGISTER_DWORD(CallFactory, CallInLogonUser, freerds::icp::LogonUser);
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif //__CALL_IN_LOGON_USER_H_
