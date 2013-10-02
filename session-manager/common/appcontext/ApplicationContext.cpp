#include "ApplicationContext.h"


namespace freerds{
	namespace sessionmanager{


	ApplicationContext::ApplicationContext(){

	}


	ApplicationContext::~ApplicationContext(){

	}

	sessionNS::SessionStore * ApplicationContext::getSessionStore(){
		return &mSessionStore;
	}

	int ApplicationContext::startRPCEngine(){
		return mRpcEngine.startEngine();
	}

	int ApplicationContext::stopRPCEngine(){
		return mRpcEngine.stopEngine();
	}


    }
}
