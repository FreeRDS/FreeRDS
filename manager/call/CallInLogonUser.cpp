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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CallInLogonUser.h"
#include <session/ApplicationContext.h>
#include <module/AuthModule.h>

namespace freerds
{
	static wLog* logger_CallInLogonUser = WLog_Get("freerds.CallInLogonUser");

	CallInLogonUser::CallInLogonUser()
	: mConnectionId(0), mAuthStatus(0), mWidth(0), mHeight(0), mColorDepth(0),
	  mClientName(), mClientAddress(), mClientBuildNumber(0), mClientProductId(0),
	  mClientHardwareId(0), mClientProtocolType(0),
	  m_RequestId(FDSAPI_LOGON_USER_REQUEST_ID), m_ResponseId(FDSAPI_LOGON_USER_RESPONSE_ID)
	{

	};

	CallInLogonUser::~CallInLogonUser()
	{

	};

	unsigned long CallInLogonUser::getCallType()
	{
		return m_RequestId;
	};

	int CallInLogonUser::decodeRequest()
	{
		BYTE* buffer;
		UINT32 length;

		buffer = (BYTE*) mEncodedRequest.data();
		length = (UINT32) mEncodedRequest.size();

		freerds_rpc_msg_unpack(m_RequestId, &m_Request, buffer, length);

		mConnectionId = m_Request.ConnectionId;
		mUserName = m_Request.User ? m_Request.User : "";
		mDomainName = m_Request.Domain ? m_Request.Domain : "";
		mPassword = m_Request.Password ? m_Request.Password : "";
		mWidth = m_Request.DesktopWidth;
		mHeight = m_Request.DesktopHeight;
		mColorDepth = m_Request.ColorDepth;
		mClientName = m_Request.ClientName ? m_Request.ClientName : "";
		mClientAddress = m_Request.ClientAddress ? m_Request.ClientAddress : "";
		mClientBuildNumber = m_Request.ClientBuild;
		mClientProductId = m_Request.ClientProductId;
		mClientHardwareId = m_Request.ClientHardwareId;
		mClientProtocolType = m_Request.ClientProtocolType;

		freerds_rpc_msg_free(m_RequestId, &m_Request);

		return 0;
	};

	int CallInLogonUser::encodeResponse()
	{
		wStream* s;

		m_Response.ServiceEndpoint = (char*) mPipeName.c_str();

		s = freerds_rpc_msg_pack(m_ResponseId, &m_Response, NULL);

		mEncodedResponse.assign((const char*) Stream_Buffer(s), Stream_Length(s));

		Stream_Free(s, TRUE);

		return 0;
	};

	int CallInLogonUser::authenticateUser()
	{
		ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getOrCreateConnection(mConnectionId);

		if (currentConnection == NULL)
		{
			WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "Cannot get Connection for connectionId %lu",mConnectionId);
			mAuthStatus = -1;
			return -1;
		}

		return currentConnection->authenticateUser(mUserName, mDomainName, mPassword);
	}

	int CallInLogonUser::getUserSession()
	{
		bool reconnectAllowed;
		SessionPtr currentSession;
		ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getOrCreateConnection(mConnectionId);

		if (!APP_CONTEXT.getPropertyManager()->getPropertyBool("session.reconnect", reconnectAllowed)) {
			reconnectAllowed = true;
		}

		if (reconnectAllowed) {
			currentSession = APP_CONTEXT.getSessionStore()->getFirstDisconnectedSessionUserName(mUserName, mDomainName);
		}

		if (currentSession && (currentSession->getConnectState() == WTSDisconnected))
		{
			// reconnect to a disconnected session
			currentSession->setClientDisplayWidth(mWidth);
			currentSession->setClientDisplayHeight(mHeight);
			currentSession->setClientDisplayColorDepth(mColorDepth);
			currentSession->setClientName(mClientName);
			currentSession->setClientAddress(mClientAddress);
			currentSession->setClientBuildNumber(mClientBuildNumber);
			currentSession->setClientProductId(mClientProductId);
			currentSession->setClientHardwareId(mClientHardwareId);
			currentSession->setClientProtocolType(mClientProtocolType);
		}
		else
		{
			// create new Session for this request
			currentSession = APP_CONTEXT.getSessionStore()->createSession();
			currentSession->setUserName(mUserName);
			currentSession->setDomain(mDomainName);
			currentSession->setClientDisplayWidth(mWidth);
			currentSession->setClientDisplayHeight(mHeight);
			currentSession->setClientDisplayColorDepth(mColorDepth);
			currentSession->setClientName(mClientName);
			currentSession->setClientAddress(mClientAddress);
			currentSession->setClientBuildNumber(mClientBuildNumber);
			currentSession->setClientProductId(mClientProductId);
			currentSession->setClientHardwareId(mClientHardwareId);
			currentSession->setClientProtocolType(mClientProtocolType);

			char winStationName[32];
			sprintf(winStationName, "RDP-Tcp#%d", mConnectionId);
			currentSession->setWinStationName(winStationName);

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

		currentConnection->setSessionId(currentSession->getSessionId());

		currentConnection->getClientInformation()->with = mWidth;
		currentConnection->getClientInformation()->height = mHeight;
		currentConnection->getClientInformation()->colordepth = mColorDepth;

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

		currentSession->setConnectState(WTSActive);

		mPipeName = currentSession->getPipeName();
		return 0;
	}

	int CallInLogonUser::getAuthSession()
	{
		// authentication failed, start up greeter module
		ConnectionPtr currentConnection = APP_CONTEXT.getConnectionStore()->getOrCreateConnection(mConnectionId);
		SessionPtr currentSession = APP_CONTEXT.getSessionStore()->createSession();

		std::string greeter;

		if (!APP_CONTEXT.getPropertyManager()->getPropertyString("auth.greeter", greeter)) {
			greeter = "Qt";
		}
		currentSession->setModuleConfigName(greeter);

		currentSession->setUserName(mUserName);
		currentSession->setDomain(mDomainName);
		currentSession->setClientDisplayWidth(mWidth);
		currentSession->setClientDisplayHeight(mHeight);
		currentSession->setClientDisplayColorDepth(mColorDepth);
		currentSession->setClientName(mClientName);
		currentSession->setClientAddress(mClientAddress);
		currentSession->setClientBuildNumber(mClientBuildNumber);
		currentSession->setClientProductId(mClientProductId);
		currentSession->setClientHardwareId(mClientHardwareId);
		currentSession->setClientProtocolType(mClientProtocolType);

		char winStationName[32];
		sprintf(winStationName, "RDP-Tcp#%d", mConnectionId);
		currentSession->setWinStationName(winStationName);

		if (!currentSession->generateAuthEnvBlockAndModify())
		{
			WLog_Print(logger_CallInLogonUser, WLOG_ERROR, "generateEnvBlockAndModify failed for user %s with domain %s",mUserName.c_str(),mDomainName.c_str());
			mResult = 1;// will report error with answer
			return 1;
		}

		currentConnection->setSessionId(currentSession->getSessionId());

		currentConnection->getClientInformation()->with = mWidth;
		currentConnection->getClientInformation()->height = mHeight;
		currentConnection->getClientInformation()->colordepth = mColorDepth;

		currentSession->setAuthSession(true);

		if (!currentSession->startModule(greeter))
		{
			mResult = 1;// will report error with answer
			return 1;
		}

		currentSession->setConnectState(WTSConnected);

		mPipeName = currentSession->getPipeName();
		return 0;
	}

	int CallInLogonUser::doStuff()
	{
		int authStatus;

		authStatus = authenticateUser();

		if (authStatus != 0)
			getAuthSession();
		else
			getUserSession();

		return 0;
	}
}
