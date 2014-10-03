/**
 * Module
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

#ifndef MODULE_H_
#define MODULE_H_

#include "Module.h"
#include <winpr/wtypes.h>
#include <freerds/module.h>
#include <string>

namespace freerds
{
	namespace sessionmanager
	{
		namespace module
		{
			class Module
			{
			public:
				Module();
				int initModule(HMODULE libHandle, std::string moduleFileName, RDS_MODULE_ENTRY_POINTS* entrypoints);
				virtual ~Module();
				std::string getName();

				RDS_MODULE_COMMON* newContext();
				void freeContext(RDS_MODULE_COMMON* context);

				std::string start(RDS_MODULE_COMMON* context);
				int stop(RDS_MODULE_COMMON* context);

			private:
				pRdsModuleNew mfpNew;
				pRdsModuleFree mfpFree;

				pRdsModuleStart mfpStart;
				pRdsModuleStop mfpStop;
				std::string mModuleFile;
				std::string mModuleName;
			};
		}
	}
}

namespace moduleNS = freerds::sessionmanager::module;

#endif /* MODULE_H_ */
