/**
 * Application Context of Session Manager
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

#ifndef __APP_CONTEXT_SESSION_MANAGER
#define __APP_CONTEXT_SESSION_MANAGER

#include <utils/SingletonBase.h>
#include <utils/SignalingQueue.h>
#include <session/SessionStore.h>
#include <session/ConnectionStore.h>
#include <pbRPC/RpcEngine.h>
#include <winpr/wlog.h>
#include <call/CallOut.h>
#include <module/ModuleManager.h>
#include <config/PropertyManager.h>
#include <task/Executor.h>

#ifdef WITH_FDSAPI
#include <fdsapi/FDSApiServer.h>
#endif

#define APP_CONTEXT freerds::sessionmanager::ApplicationContext::instance()

namespace freerds
{
	namespace sessionmanager
	{
		class ApplicationContext: public SingletonBase<ApplicationContext>
		{
		public:
			sessionNS::SessionStore* getSessionStore();
			sessionNS::ConnectionStore* getConnectionStore();
			configNS::PropertyManager* getPropertyManager();
			moduleNS::ModuleManager* getModuleManager();
			int startRPCEngine();
			int stopRPCEngine();

			void startTaskExecutor();
			void stopTaskExecutor();
			void addTask(taskNS::TaskPtr task);

			std::string getHomePath();
			std::string getLibraryPath();
			std::string getExecutablePath();
			std::string getShareDataPath();
			std::string getSystemConfigPath();

			SignalingQueue<callNS::Call *> * getRpcOutgoingQueue();

			int loadModulesFromPath(std::string path);
			void setupTestingPropValues();

		private:
			std::string mHomePath;
			std::string mLibraryPath;
			std::string mExecutablePath;
			std::string mShareDataPath;
			std::string mSystemConfigPath;

			void initPaths();

			void configureExecutableSearchPath();

			taskNS::Executor mTaskExecutor;
			sessionNS::SessionStore mSessionStore;
			sessionNS::ConnectionStore mConnectionStore;

			configNS::PropertyManager mPropertyManager;
			pbRPC::RpcEngine mRpcEngine;
			SignalingQueue<callNS::Call *> mRpcOutgoingCalls;
			wLog* mWLogRoot;
			moduleNS::ModuleManager mModuleManager;
#ifdef WITH_FDSAPI
			fdsapiNS::FDSApiServer mFDSApiServer;
#endif
			SINGLETON_ADD_INITIALISATION(ApplicationContext)
		};
	}
}

namespace appNS = freerds::sessionmanager;

#endif
