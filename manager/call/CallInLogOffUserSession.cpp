/**
 * Class for rpc call LogOffUserSession (freerds to session manager)
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

#include "CallInLogOffUserSession.h"
#include <appcontext/ApplicationContext.h>

namespace freerds
{
	CallInLogOffUserSession::CallInLogOffUserSession()
	: m_RequestId(FDSAPI_LOGOFF_USER_REQUEST_ID), m_ResponseId(FDSAPI_LOGOFF_USER_RESPONSE_ID)
	{
		mConnectionId = 0;
		mLoggedOff = false;
	};

	CallInLogOffUserSession::~CallInLogOffUserSession()
	{

	};

	unsigned long CallInLogOffUserSession::getCallType()
	{
		return m_RequestId;
	};

	int CallInLogOffUserSession::decodeRequest()
	{
		BYTE* buffer;
		UINT32 length;

		buffer = (BYTE*) mEncodedRequest.data();
		length = (UINT32) mEncodedRequest.size();

		freerds_rpc_msg_unpack(m_RequestId, &m_Request, buffer, length);

		mConnectionId = m_Request.ConnectionId;

		freerds_rpc_msg_free(m_RequestId, &m_Request);

		return 0;
	};

	int CallInLogOffUserSession::encodeResponse()
	{
		wStream* s;

		m_Response.ConnectionId = mConnectionId;

		s = freerds_rpc_msg_pack(m_ResponseId, &m_Response, NULL);

		mEncodedResponse.assign((const char*) Stream_Buffer(s), Stream_Length(s));

		Stream_Free(s, TRUE);

		return 0;
	};

	int CallInLogOffUserSession::doStuff()
	{
		ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getConnection(mConnectionId);

		if ((currentConnection == NULL) || (currentConnection->getSessionId() == 0)) {
			mLoggedOff = false;
			return -1;
		}
		SessionPtr currentSession = APP_CONTEXT.getSessionStore()->getSession(currentConnection->getSessionId());

		if (!currentSession) {
			mLoggedOff = false;
			return -1;
		}

		currentSession->stopModule();

		APP_CONTEXT.getSessionStore()->removeSession(currentConnection->getSessionId());
		APP_CONTEXT.getConnectionStore()->removeConnection(mConnectionId);
		mLoggedOff = true;
		return 0;
	}
}
