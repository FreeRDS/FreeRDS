#include "ApplicationContext.h"


namespace freeRDS{
	namespace sessionmanager{


	ApplicationContext::ApplicationContext(){

	}


	ApplicationContext::~ApplicationContext(){

	}

	sessionNS::SessionStore * ApplicationContext::getSessionStore(){
		return &mSessionStore;
	}



    }
}
