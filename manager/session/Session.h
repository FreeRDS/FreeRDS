/**
 * Session class
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

#ifndef SESSION_H_
#define SESSION_H_

#include <list>
#include <string>

#include <winpr/handle.h>
#include <winpr/wtsapi.h>

#include <freerds/module.h>

#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Connection.h"

namespace freerds
{
	class Session
	{
	public:
		Session(UINT32 sessionId);
		~Session();

		std::string getDomain();
		void setDomain(std::string domainName);
		std::string getUserName();
		void setUserName(std::string username);
		std::string getWinStationName();
		void setWinStationName(std::string winStationName);
		UINT32 getSessionId();
		std::string getPipeName();

		void setClientName(std::string name);
		std::string getClientName();
		void setClientAddress(std::string address);
		std::string getClientAddress();
		void setClientBuildNumber(UINT32 buildNumber);
		UINT32 getClientBuildNumber();
		void setClientProductId(UINT16 productId);
		UINT16 getClientProductId();
		void setClientHardwareId(UINT32 hardwareId);
		UINT32 getClientHardwareId();
		void setClientDisplayWidth(UINT32 displayWidth);
		UINT32 getClientDisplayWidth();
		void setClientDisplayHeight(UINT32 displayHeight);
		UINT32 getClientDisplayHeight();
		void setClientDisplayColorDepth(UINT32 displayColorDepth);
		UINT32 getClientDisplayColorDepth();
		void setClientProtocolType(UINT16 protocolType);
		UINT16 getClientProtocolType();

		bool generateUserToken();
		bool generateEnvBlockAndModify();
		bool generateAuthEnvBlockAndModify();

		char** getPEnvBlock();

		bool isAuthSession();
		void setAuthSession(bool authSession);
		int authenticate(std::string username, std::string domain, std::string password);

		void setModuleConfigName(std::string configName);
		std::string getModuleConfigName();
		bool startModule(std::string & pipeName);
		bool stopModule();

		WTS_CONNECTSTATE_CLASS getConnectState();
		boost::posix_time::ptime getConnectStateChangeTime();
		void setConnectState(WTS_CONNECTSTATE_CLASS state);

		UINT32 getStateChange(WTS_CONNECTSTATE_CLASS oldState, WTS_CONNECTSTATE_CLASS newState);

	private:

		char* dupEnv(char* orgBlock);

		UINT32 m_SessionId;
		bool mAuthSession;
		bool mSessionStarted;

		std::string mUsername;
		std::string mDomain;
		std::string mWinStationName;

		UINT32 mClientBuildNumber;
		std::string mClientName;
		UINT16 mClientProductId;
		UINT32 mClientHardwareId;
		std::string mClientAddress;
		UINT32 mClientDisplayWidth;
		UINT32 mClientDisplayHeight;
		UINT32 mClientDisplayColorDepth;
		UINT16 mClientProtocolType;

		std::string mPipeName;

		HANDLE mUserToken;
		char* mpEnvBlock;

		std::string mModuleConfigName;
		std::string mModuleName;
		RDS_MODULE_COMMON* mCurrentModuleContext;
		WTS_CONNECTSTATE_CLASS mCurrentState;
		boost::posix_time::ptime mCurrentStateChangeTime;
		CRITICAL_SECTION mCSection;
	};

	typedef boost::shared_ptr<Session> SessionPtr;
}

#endif
