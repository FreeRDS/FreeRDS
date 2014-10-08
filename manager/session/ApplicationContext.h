/**
 * Application Context of Session Manager
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

#ifndef __APP_CONTEXT_SESSION_MANAGER
#define __APP_CONTEXT_SESSION_MANAGER

#include <winpr/wlog.h>

#include <utils/SingletonBase.h>
#include <utils/SignalingQueue.h>

#include <session/SessionStore.h>
#include <session/ConnectionStore.h>

#include <call/CallOut.h>
#include <task/Executor.h>
#include <call/RpcEngine.h>

#include <module/ModuleManager.h>
#include <session/PropertyManager.h>

#include <fdsapi/FDSApiServer.h>

#define APP_CONTEXT freerds::ApplicationContext::instance()

namespace freerds
{
	class ApplicationContext: public SingletonBase<ApplicationContext>
	{
	public:
		SessionStore* getSessionStore();
		ConnectionStore* getConnectionStore();
		PropertyManager* getPropertyManager();
		ModuleManager* getModuleManager();
		FDSApiServer* getFDSApiServer();

		int startRPCEngine();
		int stopRPCEngine();

		void startTaskExecutor();
		void stopTaskExecutor();
		void startSessionTimoutMonitor();
		bool addTask(TaskPtr task);

		std::string getHomePath();
		std::string getLibraryPath();
		std::string getExecutablePath();
		std::string getShareDataPath();
		std::string getSystemConfigPath();

		SignalingQueue<Call*>* getRpcOutgoingQueue();

		int loadModulesFromPath(std::string path);
		void setupTestingPropValues();

		void rpcDisconnected();

	private:
		std::string mHomePath;
		std::string mLibraryPath;
		std::string mExecutablePath;
		std::string mShareDataPath;
		std::string mSystemConfigPath;

		void initPaths();
		void exportContext();

		void configureExecutableSearchPath();

		wLog* mWLogRoot;

		Executor mTaskExecutor;
		SessionStore mSessionStore;
		ConnectionStore mConnectionStore;

		PropertyManager mPropertyManager;
		RpcEngine mRpcEngine;
		SignalingQueue<Call*> mRpcOutgoingCalls;

		ModuleManager mModuleManager;
		FDSApiServer mFDSApiServer;

		SINGLETON_ADD_INITIALISATION(ApplicationContext)
	};
}

#endif
