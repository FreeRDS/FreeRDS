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

#include <winpr/crt.h>
#include <winpr/tchar.h>

#include <module/AuthModule.h>
#include <appcontext/ApplicationContext.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{
			static wLog* logger_Session= WLog_Get("freerds.sessionmanager.session.session");

			Session::Session(long sessionID): mSessionID(sessionID),
					mSessionStarted(false), mpEnvBlock(NULL),
					mCurrentState(WTSDown), mUserToken(NULL),
					mCurrentModuleContext(NULL), mAuthSession(false)
			{
				if (!InitializeCriticalSectionAndSpinCount(&mCSection, 0x00000400))
				{
					 WLog_Print(logger_Session, WLOG_FATAL, "cannot init Session critical section!");
				}

			}

			Session::~Session()
			{

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

			UINT32 Session::getSessionID()
			{
				return mSessionID;
			}

			bool Session::isAuthSession() {
				return mAuthSession;
			}

			void Session::setAuthSession(bool authSession) {
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

			std::string Session::getModuleConfigName() {
				return mModuleConfigName;
			}

			char * Session::dupEnv(char * orgBlock) {

				char * penvb = orgBlock;
				int length;
				int currentLength;
				char *lpszEnvironmentBlock;
				if (orgBlock == NULL) {
					return NULL;
				} else {
					length = 0;
					while (*penvb && *(penvb+1))
					{
						currentLength = strlen(penvb) + 1;
						length += currentLength;
						penvb += (currentLength);
					}
					lpszEnvironmentBlock = (char *) malloc((length + 2) * sizeof(CHAR));
					memcpy(lpszEnvironmentBlock,orgBlock,length + 1);

				}
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
				if (!APP_CONTEXT.getPropertyManager()->getPropertyString(mSessionID,queryString,mModuleName)) {
					WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, Property %s not found.",queryString.c_str());
					return false;
				}

				moduleNS::Module* currentModule = APP_CONTEXT.getModuleManager()->getModule(mModuleName);

				if (!currentModule)
				{
					WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, no module found for name %s", mModuleName.c_str());
					return false;
				}

				mCurrentModuleContext = currentModule->newContext();
				mCurrentModuleContext->sessionId = mSessionID;

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
					setConnectState(WTSActive);
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

				moduleNS::Module* currentModule = APP_CONTEXT.getModuleManager()->getModule(mModuleName);

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
				mCurrentState = state;
				mCurrentStateChangeTime = boost::date_time::second_clock<boost::posix_time::ptime>::universal_time();
			}

			boost::posix_time::ptime Session::getConnectStateChangeTime() {
				return mCurrentStateChangeTime;
			}
		}
	}
}


