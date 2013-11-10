/**
 * Factory for rpc calls
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

#ifndef __CALL_FACTORY_HEADER
#define __CALL_FACTORY_HEADER

#include <utils/FactoryBase.h>
#include <utils/SingletonBase.h>
#include <call/Call.h>

#include <string>

#define CALL_FACTORY FreeRDS::SessionManager::call::CallFactory::instance()

namespace FreeRDS
{
	namespace SessionManager
	{
		namespace call
		{

		/**
		* @class	PacketFactory.
		*
		* @brief	Factory for creating packetes.
		*
		* @author	Martin Haimberger
		*/

		class CallFactory :public FactoryBase<Call,unsigned long>, public SingletonBase<CallFactory>
		{
			SINGLETON_ADD_INITIALISATION(CallFactory)
		};

		}
	}
}

namespace callNS = FreeRDS::SessionManager::call;

#endif
