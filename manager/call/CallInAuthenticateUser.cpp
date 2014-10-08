/**
 * Class for rpc call LogonUser (freerds to session manager)
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

#include "CallInAuthenticateUser.h"
#include "TaskSwitchTo.h"

#include <session/ApplicationContext.h>

namespace freerds
{
	static wLog* logger_CallInLogonUser = WLog_Get("freerds.CallInAuthenticateUser");

	CallInAuthenticateUser::CallInAuthenticateUser()
	: mSessionId(0), mAuthStatus(0),
	  m_RequestId(FDSAPI_START_SESSION_REQUEST_ID), m_ResponseId(FDSAPI_START_SESSION_RESPONSE_ID)
	{

	};

	CallInAuthenticateUser::~CallInAuthenticateUser()
	{

	};

	unsigned long CallInAuthenticateUser::getCallType()
	{
		return m_RequestId;
	};

	int CallInAuthenticateUser::decodeRequest()
	{
		BYTE* buffer;
		UINT32 length;

		buffer = (BYTE*) mEncodedRequest.data();
		length = (UINT32) mEncodedRequest.size();

		freerds_rpc_msg_unpack(m_RequestId, &m_Request, buffer, length);

		mSessionId = m_Request.SessionId;
		mUserName = m_Request.User ? m_Request.User : "";
		mDomainName = m_Request.Domain ? m_Request.Domain : "";
		mPassword = m_Request.Password ? m_Request.Password : "";

		freerds_rpc_msg_free(m_RequestId, &m_Request);

		return 0;
	};

	int CallInAuthenticateUser::encodeResponse()
	{
		wStream* s;

		m_Response.ServiceEndpoint = NULL;
		m_Response.status = (mAuthStatus == 0) ? 0 : 1;

		s = freerds_rpc_msg_pack(m_ResponseId, &m_Response, NULL);

		mEncodedResponse.assign((const char*) Stream_Buffer(s), Stream_Length(s));

		Stream_Free(s, TRUE);

		return 0;
	};

	int CallInAuthenticateUser::authenticateUser()
	{
		UINT32 connectionId = APP_CONTEXT.getConnectionStore()->getConnectionIdForSessionId(mSessionId);
		ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getConnection(connectionId);

		if (currentConnection == NULL) {
			WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "Cannot get Connection for sessionId %lu for resolved connectionId %lu",mSessionId,connectionId);
			mAuthStatus = -1;
			return -1;
		}

		mAuthStatus = currentConnection->authenticateUser(mUserName, mDomainName, mPassword);
		return mAuthStatus;
	}

	int CallInAuthenticateUser::getUserSession()
	{
		SessionPtr currentSession;
		bool reconnectAllowed;

		if (!APP_CONTEXT.getPropertyManager()->getPropertyBool("session.reconnect", reconnectAllowed)) {
			reconnectAllowed = true;
		}

		if (reconnectAllowed) {
			currentSession = APP_CONTEXT.getSessionStore()->getFirstDisconnectedSessionUserName(mUserName, mDomainName);
		}

		if ((!currentSession) || (currentSession->getConnectState() != WTSDisconnected))
		{
			// create new Session for this request
			currentSession = APP_CONTEXT.getSessionStore()->createSession();
			currentSession->setUserName(mUserName);
			currentSession->setDomain(mDomainName);

			if (!currentSession->generateUserToken())
			{
				WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "generateUserToken failed for user %s with domain %s",mUserName.c_str(),mDomainName.c_str());
				mResult = 1;// will report error with answer
				return 1;
			}

			if (!currentSession->generateEnvBlockAndModify())
			{
				WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "generateEnvBlockAndModify failed for user %s with domain %s",mUserName.c_str(),mDomainName.c_str());
				mResult = 1;// will report error with answer
				return 1;
			}
			std::string moduleConfigName;

			if (!APP_CONTEXT.getPropertyManager()->getPropertyString("module", moduleConfigName)) {
				moduleConfigName = "X11";
			}
			currentSession->setModuleConfigName(moduleConfigName);
		}
		else
		{
			currentSession->setConnectState(WTSConnectQuery);
		}

		UINT32 connectionId = APP_CONTEXT.getConnectionStore()->getConnectionIdForSessionId(mSessionId);
		APP_CONTEXT.getConnectionStore()->getOrCreateConnection(connectionId)->setAbout2SwitchSessionId(currentSession->getSessionId());

		if (currentSession->getConnectState() == WTSDown)
		{
			std::string pipeName;
			if (!currentSession->startModule(pipeName))
			{
				WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "ModuleConfig %s does not start properly for user %s in domain %s",currentSession->getModuleConfigName().c_str(),mUserName.c_str(),mDomainName.c_str());
				mResult = 1;// will report error with answer
				return 1;
			}
		}

		TaskSwitchToPtr switchToTask = TaskSwitchToPtr(new TaskSwitchTo());
		switchToTask->setConnectionId(connectionId);
		switchToTask->setServiceEndpoint(currentSession->getPipeName());
		switchToTask->setOldSessionId(mSessionId);
		switchToTask->setNewSessionId(currentSession->getSessionId());
		APP_CONTEXT.addTask(switchToTask);

		return 0;
	}

	int CallInAuthenticateUser::doStuff()
	{
		if (authenticateUser() == 0) {
			if (mAuthStatus == 0) {
				return getUserSession();
			}
		}
		return 0;
	}
}
