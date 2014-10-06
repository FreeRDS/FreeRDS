/**
 * Class for rpc call LogOffUserSession (session manager to freerds)
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

#include "CallOutLogOffUserSession.h"
#include <appcontext/ApplicationContext.h>

using freerds::icp::LogOffUserSessionRequest;
using freerds::icp::LogOffUserSessionResponse;

namespace freerds
{
	namespace call
	{
		CallOutLogOffUserSession::CallOutLogOffUserSession()
		: m_RequestId(FDSAPI_LOGOFF_USER_REQUEST_ID), m_ResponseId(FDSAPI_LOGOFF_USER_RESPONSE_ID)
		{
			mConnectionId = 0;
			mLoggedOff = false;
		};

		CallOutLogOffUserSession::~CallOutLogOffUserSession()
		{

		};

		unsigned long CallOutLogOffUserSession::getCallType()
		{
			return m_RequestId;
		};

		int CallOutLogOffUserSession::encodeRequest()
		{
			wStream* s;

			m_Request.ConnectionId = mConnectionId;

			s = freerds_rpc_msg_pack(m_RequestId, &m_Request, NULL);

			mEncodedResponse.assign((const char*) Stream_Buffer(s), Stream_Length(s));

			Stream_Free(s, TRUE);

			return 0;
		};

		int CallOutLogOffUserSession::decodeResponse()
		{
			BYTE* buffer;
			UINT32 length;

			buffer = (BYTE*) mEncodedRequest.data();
			length = (UINT32) mEncodedRequest.size();

			freerds_rpc_msg_unpack(m_ResponseId, &m_Response, buffer, length);

			mLoggedOff = true;

			freerds_rpc_msg_free(m_RequestId, &m_Request);

			return 0;
		};

		void CallOutLogOffUserSession::setConnectionId(long connectionId) {
			mConnectionId = connectionId;
		}

		bool CallOutLogOffUserSession::isLoggedOff() {
			return mLoggedOff;
		}
	}
}
