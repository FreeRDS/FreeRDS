/**
 * Module Manager
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

#ifndef MODULEMANAGER_H_
#define MODULEMANAGER_H_

#include "Module.h"
#include <string>
#include <map>

#define MODULE_ENV_VAR "FREERDS_ADDITIONAL_MODULES"

namespace freerds{
	namespace sessionmanager{
		namespace module{
			class ModuleManager {
			public:
				ModuleManager();
				virtual ~ModuleManager();

				int loadModulesFromPath(std::string path,std::string pattern);
				int loadModuelsFromPathAndEnv(std::string path,std::string pattern);

				Module * getModule(std::string moduleName);
			private:
				int addModule(std::string path, std::string modulename);
				std::map<std::string,Module *> mModulesMap;
			};

		}
	}
}
namespace moduleNS = freerds::sessionmanager::module;

#endif /* MODULEMANAGER_H_ */
