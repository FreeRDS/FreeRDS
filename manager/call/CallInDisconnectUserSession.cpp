/**
 * Class for rpc call DisconnectUserSession (freerds to session manager)
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

#include "CallInDisconnectUserSession.h"
#include <appcontext/ApplicationContext.h>
#include <call/TaskEndSession.h>

namespace freerds
{
	CallInDisconnectUserSession::CallInDisconnectUserSession()
	: m_RequestId(FDSAPI_DISCONNECT_USER_REQUEST_ID), m_ResponseId(FDSAPI_DISCONNECT_USER_RESPONSE_ID)
	{
		mConnectionId = 0;
		mDisconnected = false;
	};

	CallInDisconnectUserSession::~CallInDisconnectUserSession()
	{

	};

	unsigned long CallInDisconnectUserSession::getCallType()
	{
		return m_RequestId;
	};

	int CallInDisconnectUserSession::decodeRequest()
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

	int CallInDisconnectUserSession::encodeResponse()
	{
		wStream* s;

		m_Response.ConnectionId = mConnectionId;

		s = freerds_rpc_msg_pack(m_ResponseId, &m_Response, NULL);

		mEncodedResponse.assign((const char*) Stream_Buffer(s), Stream_Length(s));

		Stream_Free(s, TRUE);

		return 0;
	};

	int CallInDisconnectUserSession::doStuff()
	{
		ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getConnection(mConnectionId);

		if ((currentConnection == NULL) || (currentConnection->getSessionId() == 0)) {
			mDisconnected = false;
			return -1;
		}

		SessionPtr currentSession = APP_CONTEXT.getSessionStore()->getSession(currentConnection->getSessionId());

		if (!currentSession)
		{
			mDisconnected = false;
			return -1;
		}

		currentSession->setConnectState(WTSDisconnected);
		APP_CONTEXT.getConnectionStore()->removeConnection(mConnectionId);

		long timeout;

		if (!APP_CONTEXT.getPropertyManager()->getPropertyNumber("session.timeout", &timeout)) {
			timeout = 0;
		}

		if (timeout == 0)  {
			TaskEndSessionPtr task = TaskEndSessionPtr(new TaskEndSession());
			task->setSessionId(currentSession->getSessionID());
			APP_CONTEXT.addTask(task);
		}

		mDisconnected = true;
		return 0;
	}
}
