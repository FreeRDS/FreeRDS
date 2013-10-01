#ifndef __CALL_FACTORY_HEADER
#define __CALL_FACTORY_HEADER

#include <utils/FactoryBase.h>
#include <utils/SingletonBase.h>
#include <call/Call.h>

#include <string>

#define CALL_FACTORY freeRDS::sessionmanager::call::CallFactory::instance()

namespace freeRDS{
	namespace sessionmanager{
		namespace call{

		/**
		* @class	PacketFactory.
		*
		* @brief	Factory for creating packetes.
		*
		* @author	Martin Haimberger
		*/

		class CallFactory :public FactoryBase<Call,unsigned long>, public SingletonBase<CallFactory>  {

			SINGLETON_ADD_INITIALISATION(CallFactory)
		};

		}
	}
}

namespace callNS = freeRDS::sessionmanager::call;

#endif
