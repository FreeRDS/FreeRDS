#ifndef __APP_CONTEXT_SESSION_MANAGER
#define __APP_CONTEXT_SESSION_MANAGER

#include <utils/SingletonBase.h>
#include <session/SessionStore.h>
#include <pbRPC/RpcEngine.h>

#define APP_CONTEXT freerds::sessionmanager::ApplicationContext::instance()

namespace freerds{
	namespace sessionmanager{


		class ApplicationContext: public SingletonBase<ApplicationContext>
		{
		public:
			sessionNS::SessionStore * getSessionStore();
			int startRPCEngine();
			int stopRPCEngine();
		private:
			sessionNS::SessionStore mSessionStore;
			pbRPC::RpcEngine mRpcEngine;
			SINGLETON_ADD_INITIALISATION(ApplicationContext)
		};
	
	} // namespace freeRDS end
} // namespace sessionmanager end

namespace appNS = freerds::sessionmanager;

#endif
