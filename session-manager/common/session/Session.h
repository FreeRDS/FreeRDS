/**
 * Session class
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

#ifndef SESSION_H_
#define SESSION_H_

#include <config.h>
#include <string>
#include <list>

#include <winpr/handle.h>
#include <freerds/module.h>
#include <winpr/wtsapi.h>
#include <boost/shared_ptr.hpp>

#include "Connection.h"

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{
		class Session
		{
		public:
			Session(long sessionID);
			~Session();

			std::string getDomain();
			void setDomain(std::string domainName);
			std::string getUserName();
			void setUserName(std::string username);
			UINT32 getSessionID();
			std::string getPipeName();

			bool generateUserToken();
			bool generateEnvBlockAndModify();
			char** getPEnvBlock();

			bool isAuthSession();
			void setAuthSession(bool authSession);
			int authenticate(std::string username, std::string domain, std::string password);

			void setModuleName(std::string moduleName);
			std::string getModuleName();
			bool startModule(std::string & pipeName);
			bool stopModule();

			WTS_CONNECTSTATE_CLASS getConnectState();
			void setConnectState(WTS_CONNECTSTATE_CLASS state);

		private:

			char * dupEnv(char * orgBlock);

			UINT32 mSessionID;
			bool mAuthSession;
			bool mSessionStarted;

			std::string mUsername;
			std::string mDomain;

			std::string mPipeName;

			HANDLE mUserToken;
			char* mpEnvBlock;

			std::string mModuleName;
			RDS_MODULE_COMMON* mCurrentModuleContext;
			WTS_CONNECTSTATE_CLASS mCurrentState;
			CRITICAL_SECTION mCSection;
		};

		typedef boost::shared_ptr<Session> SessionPtr;

		}
	}
}



namespace sessionNS = freerds::sessionmanager::session;

#endif
