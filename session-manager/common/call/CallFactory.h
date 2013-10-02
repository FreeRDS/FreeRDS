#ifndef __CALL_FACTORY_HEADER
#define __CALL_FACTORY_HEADER

#include <utils/FactoryBase.h>
#include <utils/SingletonBase.h>
#include <call/Call.h>

#include <string>

#define CALL_FACTORY freerds::sessionmanager::call::CallFactory::instance()

namespace freerds{
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

namespace callNS = freerds::sessionmanager::call;

#endif
