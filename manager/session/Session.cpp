/**
 * Session store class
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

#include "Session.h"

#include <string.h>
#include <pwd.h>

#include <winpr/wlog.h>
#include <winpr/sspicli.h>
#include <winpr/environment.h>
#include <winpr/wnd.h>

#include <winpr/crt.h>
#include <winpr/tchar.h>

#include <module/AuthModule.h>
#include <fdsapi/FDSApiServer.h>
#include <appcontext/ApplicationContext.h>

namespace freerds
{
	static wLog* logger_Session= WLog_Get("freerds.Session");

	Session::Session(UINT32 sessionId)
	: m_SessionId(sessionId),
	  mSessionStarted(false), mpEnvBlock(NULL),
	  mCurrentState(WTSDown), mUserToken(NULL),
	  mCurrentModuleContext(NULL), mAuthSession(false),
	  mClientDisplayColorDepth(0), mClientDisplayWidth(0), mClientDisplayHeight(0),
	  mClientHardwareId(0), mClientBuildNumber(0), mClientProductId(0), mClientProtocolType(0)
	{
		if (!InitializeCriticalSectionAndSpinCount(&mCSection, 0x00000400))
		{
			 WLog_Print(logger_Session, WLOG_FATAL, "cannot init Session critical section!");
		}

		// Fire a session created event.
		FDSApiServer* FDSApiServer = APP_CONTEXT.getFDSApiServer();
		FDSApiServer->fireSessionEvent(m_SessionId, WTS_SESSION_CREATE);
	}

	Session::~Session()
	{
		// Mark the session as down.
		setConnectState(WTSDown);

		// Fire a session terminated event.
		FDSApiServer* FDSApiServer = APP_CONTEXT.getFDSApiServer();
		FDSApiServer->fireSessionEvent(m_SessionId, WTS_SESSION_TERMINATE);
	}

	std::string Session::getDomain()
	{
		return mDomain;
	}

	void Session::setDomain(std::string domainName)
	{
		mDomain = domainName;
	}

	std::string Session::getUserName()
	{
		return mUsername;
	}

	void Session::setUserName(std::string username)
	{
		mUsername = username;
	}

	std::string Session::getWinStationName()
	{
		return mWinStationName;
	}

	void Session::setWinStationName(std::string winStationName)
	{
		mWinStationName = winStationName;
	}

	UINT32 Session::getSessionId()
	{
		return m_SessionId;
	}

	std::string Session::getClientName()
	{
		return mClientName;
	}

	void Session::setClientName(std::string name)
	{
		mClientName = name;
	}

	std::string Session::getClientAddress()
	{
		return mClientAddress;
	}

	void Session::setClientAddress(std::string address)
	{
		mClientAddress = address;
	}

	UINT32 Session::getClientBuildNumber()
	{
		return mClientBuildNumber;
	}

	void Session::setClientBuildNumber(UINT32 buildNumber)
	{
		mClientBuildNumber = buildNumber;
	}

	UINT16 Session::getClientProductId()
	{
		return mClientProductId;
	}

	void Session::setClientProductId(UINT16 productId)
	{
		mClientProductId = productId;
	}

	UINT32 Session::getClientHardwareId()
	{
		return mClientHardwareId;
	}

	void Session::setClientHardwareId(UINT32 hardwareId)
	{
		mClientHardwareId = hardwareId;
	}

	UINT32 Session::getClientDisplayWidth()
	{
		return mClientDisplayWidth;
	}

	void Session::setClientDisplayWidth(UINT32 displayWidth)
	{
		mClientDisplayWidth = displayWidth;
	}

	UINT32 Session::getClientDisplayHeight()
	{
		return mClientDisplayHeight;
	}

	void Session::setClientDisplayHeight(UINT32 displayHeight)
	{
		mClientDisplayHeight = displayHeight;
	}

	UINT32 Session::getClientDisplayColorDepth()
	{
		return mClientDisplayColorDepth;
	}

	void Session::setClientDisplayColorDepth(UINT32 displayColorDepth)
	{
		mClientDisplayColorDepth = displayColorDepth;
	}

	UINT16 Session::getClientProtocolType()
	{
		return mClientProtocolType;
	}

	void Session::setClientProtocolType(UINT16 protocolType)
	{
		mClientProtocolType = protocolType;
	}

	bool Session::isAuthSession()
	{
		return mAuthSession;
	}

	void Session::setAuthSession(bool authSession)
	{
		mAuthSession = authSession;
	}

	bool Session::generateUserToken()
	{
		if (mUsername.length() == 0)
		{
			 WLog_Print(logger_Session, WLOG_ERROR, "generate UserToken failed, no username!");
			return false;
		}

		return LogonUserA(mUsername.c_str(), mDomain.c_str(), NULL,
				LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &mUserToken);
	}

	bool Session::generateEnvBlockAndModify()
	{
		struct passwd* pwnam;
		char envstr[256];

		if (mUsername.length() == 0)
		{
			 WLog_Print(logger_Session, WLOG_ERROR, "generateEnvBlockAndModify failed, no username!");
			return false;
		}

		if (mpEnvBlock)
		{
			free(mpEnvBlock);
			mpEnvBlock = NULL;
		}

		pwnam = getpwnam(mUsername.c_str());

		if (!pwnam)
		{
			 WLog_Print(logger_Session, WLOG_ERROR, "generateEnvBlockAndModify failed to get user information (getpwnam) for username %s!",mUsername.c_str());
			return false;
		}

		sprintf_s(envstr, sizeof(envstr), "%d", (int) pwnam->pw_uid);
		SetEnvironmentVariableEBA(&mpEnvBlock, "UID", envstr);
		SetEnvironmentVariableEBA(&mpEnvBlock, "SHELL", pwnam->pw_shell);
		SetEnvironmentVariableEBA(&mpEnvBlock, "USER", pwnam->pw_name);
		SetEnvironmentVariableEBA(&mpEnvBlock, "HOME", pwnam->pw_dir);

		return true;
	}

	bool Session::generateAuthEnvBlockAndModify()
	{
		struct passwd* pwnam;
		char envstr[256];

		if (mpEnvBlock)
		{
			free(mpEnvBlock);
			mpEnvBlock = NULL;
		}

		if (mUsername.length() != 0)
		{
			SetEnvironmentVariableEBA(&mpEnvBlock, "FREERDS_USER", mUsername.c_str());
		}

		if (mDomain.length() != 0)
		{
			SetEnvironmentVariableEBA(&mpEnvBlock, "FREERDS_DOMAIN", mDomain.c_str());
		}

		return true;
	}

	char** Session::getPEnvBlock()
	{
		return &mpEnvBlock;
	}

	void Session::setModuleConfigName(std::string configName)
	{
		mModuleConfigName = configName;
	}

	std::string Session::getModuleConfigName()
	{
		return mModuleConfigName;
	}

	char* Session::dupEnv(char* orgBlock)
	{
		char* penvb = orgBlock;
		int length;
		int currentLength;
		char* lpszEnvironmentBlock;

		if (!orgBlock)
			return NULL;

		length = 0;

		while (*penvb && *(penvb+1))
		{
			currentLength = strlen(penvb) + 1;
			length += currentLength;
			penvb += (currentLength);
		}

		lpszEnvironmentBlock = (char*) malloc((length + 2) * sizeof(CHAR));
		memcpy(lpszEnvironmentBlock,orgBlock,length + 1);

		return lpszEnvironmentBlock;
	}

	bool Session::startModule(std::string& pipeName)
	{
		std::string pName;

		if (mSessionStarted)
		{
			WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, session has already be started, stop first.");
			return false;
		}

		std::string configBaseName = std::string("module.")+mModuleConfigName;
		std::string queryString = configBaseName+std::string(".modulename");

		if (!APP_CONTEXT.getPropertyManager()->getPropertyString(queryString,mModuleName)) {
			WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, Property %s not found.",queryString.c_str());
			return false;
		}

		Module* currentModule = APP_CONTEXT.getModuleManager()->getModule(mModuleName);

		if (!currentModule)
		{
			WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, no module found for name %s", mModuleName.c_str());
			return false;
		}

		mCurrentModuleContext = currentModule->newContext();
		mCurrentModuleContext->sessionId = m_SessionId;

		mCurrentModuleContext->userName = _strdup(mUsername.c_str());

		mCurrentModuleContext->userToken = mUserToken;
		mCurrentModuleContext->envBlock = dupEnv(mpEnvBlock);
		mCurrentModuleContext->baseConfigPath = _strdup(configBaseName.c_str());

		pName = currentModule->start(mCurrentModuleContext);

		if (pName.length() == 0)
		{
			WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, no pipeName was returned");
			return false;
		}
		else
		{
			pipeName = pName;
			mPipeName = pName;
			mSessionStarted = true;
			setConnectState(WTSConnected);
			return true;
		}
	}

	bool Session::stopModule()
	{
		if (!mSessionStarted)
		{
			WLog_Print(logger_Session, WLOG_ERROR, "stopModule failed, no session has started yet.");
			return false;
		}

		Module* currentModule = APP_CONTEXT.getModuleManager()->getModule(mModuleName);

		if (!currentModule)
		{
			WLog_Print(logger_Session, WLOG_ERROR, "stopModule failed, no module found for name %s", mModuleName.c_str());
			return false;
		}

		currentModule->stop(mCurrentModuleContext);

		currentModule->freeContext(mCurrentModuleContext);
		mCurrentModuleContext = NULL;
		mPipeName.clear();
		setConnectState(WTSDown);

		return true;
	}

	std::string Session::getPipeName()
	{
		return mPipeName;
	}

	WTS_CONNECTSTATE_CLASS Session::getConnectState()
	{
		return mCurrentState;
	}

	void Session::setConnectState(WTS_CONNECTSTATE_CLASS state)
	{
		FDSApiServer* FDSApiServer = APP_CONTEXT.getFDSApiServer();

		if (mCurrentState != state)
		{
			UINT32 stateChange = getStateChange(mCurrentState, state);

			mCurrentState = state;
			mCurrentStateChangeTime = boost::date_time::second_clock<boost::posix_time::ptime>::universal_time();

			if ((state != WTSConnected) && (state != WTSActive))
			{
				// Clear out the client information.
				setClientName("");
				setClientAddress("");
				setClientBuildNumber(0);
				setClientProductId(0);
				setClientHardwareId(0);
				setClientProtocolType(0);
			}

			if (stateChange != 0)
			{
				// Fire a session state change event.
				FDSApiServer->fireSessionEvent(m_SessionId, stateChange);
			}
		}
	}

	boost::posix_time::ptime Session::getConnectStateChangeTime()
	{
		return mCurrentStateChangeTime;
	}

	UINT32 Session::getStateChange(WTS_CONNECTSTATE_CLASS oldState, WTS_CONNECTSTATE_CLASS newState)
	{
		// Based on the transition from the old state to the new state,
		// this method returns the corresponding state change (if any).
		// These are the same state change notifications that can occur
		// as a result of a WM_WTSSESSION_CHANGE message.

		UINT32 stateChange = 0;

		switch (newState)
		{
			case WTSConnected:
				if (newState != oldState)
				{
					stateChange = WTS_REMOTE_CONNECT;
				}
				break;

			case WTSDisconnected:
				if (newState != oldState)
				{
					stateChange = WTS_REMOTE_DISCONNECT;
				}
				break;

			case WTSActive:
				if (newState != oldState)
				{
					stateChange = WTS_SESSION_LOGON;
				}
				break;

			default:
				if ((oldState == WTSActive) || (oldState == WTSDisconnected))
				{
					stateChange = WTS_SESSION_LOGOFF;
				}
				break;
		}

		return stateChange;
	}
}
