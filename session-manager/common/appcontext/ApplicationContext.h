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
#include <pbRPC/RpcEngine.h>
#include <winpr/wlog.h>
#include <call/CallOut.h>
#include <module/ModuleManager.h>
#include <config/PropertyManager.h>

#ifdef WITH_FDSAPI
#include <fdsapi/FDSApiServer.h>
#endif


#define APP_CONTEXT freerds::sessionmanager::ApplicationContext::instance()

namespace freerds{
	namespace sessionmanager{


		class ApplicationContext: public SingletonBase<ApplicationContext>
		{
		public:
			sessionNS::SessionStore * getSessionStore();
			configNS::PropertyManager * getPropertyManager();
			moduleNS::ModuleManager * getModuleManager();
			int startRPCEngine();
			int stopRPCEngine();

			SignalingQueue<callNS::Call> * getRpcOutgoingQueue();

			int loadModulesFromPath(std::string path);

			void setupTestingPropValues();


		private:
			sessionNS::SessionStore mSessionStore;
			configNS::PropertyManager mPropertyManager;
			pbRPC::RpcEngine mRpcEngine;
			SignalingQueue<callNS::Call> mRpcOutgoingCalls;
			wLog* mWLogRoot;
			moduleNS::ModuleManager mModuleManager;
#ifdef WITH_FDSAPI
			fdsapiNS::FDSApiServer mFDSApiServer;
#endif
			SINGLETON_ADD_INITIALISATION(ApplicationContext)
		};
	
	} // namespace freeRDS end
} // namespace sessionmanager end

namespace appNS = freerds::sessionmanager;

#endif
