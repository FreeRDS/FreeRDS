/**
 * Session store class
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

#include <appcontext/ApplicationContext.h>


namespace freerds{
	namespace sessionmanager{
		namespace session{

		static wLog * logger_Session= WLog_Get("freerds.sessionmanager.session.session");

			Session::Session(long sessionID):mSessionID(sessionID),mSessionStarted(false),mpEnvBlock(NULL) {

			}

			Session::~Session() {

			}

			std::string Session::getDomain() {
				return mDomain;
			}

			void Session::setDomain(std::string domain) {
				mDomain = domain;
			}

			std::string Session::getUserName() {
				return mUsername;
			}

			void Session::setUserName(std::string username) {
				mUsername = username;
			}

			long Session::getSessionID() {
				return mSessionID;
			}

			bool Session::generateUserToken() {
				if (mUsername.length() == 0 ) {
					 WLog_Print(logger_Session, WLOG_ERROR, "generate UserToken failed, no username!");
					return false;
				};
				return LogonUserA(mUsername.c_str(), mDomain.c_str(), NULL,
						LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &mUserToken);
			}

			bool Session::generateEnvBlockAndModify() {
				struct passwd* pwnam;
				char envstr[256];

				if (mUsername.length() == 0 ) {
					 WLog_Print(logger_Session, WLOG_ERROR, "generateEnvBlockAndModify failed, no username!");
					return false;
				};

				if (mpEnvBlock){
					free(mpEnvBlock);
					mpEnvBlock = NULL;
				}

				pwnam = getpwnam(mUsername.c_str());
				if (pwnam == NULL) {
					 WLog_Print(logger_Session, WLOG_ERROR, "generateEnvBlockAndModify failed to get userinformation (getpwnam) for username %s!",mUsername.c_str());
					return false;
				}

				sprintf_s(envstr, sizeof(envstr), "%d", (int) pwnam->pw_uid);
				SetEnvironmentVariableEBA(&mpEnvBlock,"UID",envstr);
				SetEnvironmentVariableEBA(&mpEnvBlock,"SHELL",pwnam->pw_shell);
				SetEnvironmentVariableEBA(&mpEnvBlock,"USER",pwnam->pw_name);
				SetEnvironmentVariableEBA(&mpEnvBlock,"HOME",pwnam->pw_dir);
				return true;
			}

			char** Session::getPEnvBlock() {
				return &mpEnvBlock;
			}

			void Session::setModuleName(std::string moduleName) {
				mModuleName = moduleName;
			}


			bool Session::startModule(std::string& pipeName) {

				std::string pName;

				if (mSessionStarted) {
					WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, session has already be started, stop first.");
					return false;
				}
				moduleNS::Module * currentModule = APP_CONTEXT.getModuleManager()->getModule(mModuleName);
				if (currentModule == NULL) {
					WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, no module found for name %s",mModuleName.c_str());
					return false;
				}

				mCurrentModuleContext = currentModule->newContext();
				mCurrentModuleContext->sessionId = mSessionID;

				mCurrentModuleContext->userName = strdup(mUsername.c_str());

				mCurrentModuleContext->userToken = mUserToken;
				mCurrentModuleContext->envBlock = &mpEnvBlock;

				pName = currentModule->start(mCurrentModuleContext);
				if (pName.length() == 0) {
					WLog_Print(logger_Session, WLOG_ERROR, "startModule failed, no pipeName was returned");
					return false;
				} else {
					pipeName = pName;
					mPipeName = pName;
					mSessionStarted = true;
					setConnectState(WTSActive);
					return true;
				}
			}

			bool Session::stopModule() {
				if (!mSessionStarted) {
					WLog_Print(logger_Session, WLOG_ERROR, "stopModule failed, no session has started yet.");
					return false;
				}
				moduleNS::Module * currentModule = APP_CONTEXT.getModuleManager()->getModule(mModuleName);
				if (currentModule == NULL) {
					WLog_Print(logger_Session, WLOG_ERROR, "stopModule failed, no module found for name %s",mModuleName.c_str());
					return false;
				}

				currentModule->stop(mCurrentModuleContext);

				currentModule->freeContext(mCurrentModuleContext);
				mCurrentModuleContext = NULL;
				mPipeName.clear();
				setConnectState(WTSDown);
				return true;

			}

			WTS_CONNECTSTATE_CLASS Session::getConnectState() {
				return mCurrentState;
			}

			void Session::setConnectState(WTS_CONNECTSTATE_CLASS state) {
				mCurrentState = state;
			}


		}
	}
}

