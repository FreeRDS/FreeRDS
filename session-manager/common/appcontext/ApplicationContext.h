#ifndef __APP_CONTEXT_SESSION_MANAGER
#define __APP_CONTEXT_SESSION_MANAGER

#include <utils/SingletonBase.h>
#include <session/SessionStore.h>

#define APP_CONTEXT freeRDS::sessionmanager::ApplicationContextSessionManager::instance()

namespace freeRDS{
	namespace sessionmanager{


		class ApplicationContext: public SingletonBase<ApplicationContext>
		{
		public:
			sessionNS::SessionStore * getSessionStore();
		private:
			sessionNS::SessionStore mSessionStore;
			SINGLETON_ADD_INITIALISATION(ApplicationContext)
		};
	
	} // namespace freeRDS end
} // namespace sessionmanager end

namespace appNS = freeRDS::sessionmanager;

#endif
