/**
 * Module
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

#ifndef AUTH_MODULE_H_
#define AUTH_MODULE_H_

#include "Module.h"

#include <freerds/auth.h>

#include <winpr/wtypes.h>
#include <freerds/modules.h>
#include <string>

namespace freerds
{
	namespace sessionmanager
	{
		namespace module
		{
			class AuthModule
			{
			public:
				AuthModule();
				int initModule(pRdsAuthModuleEntry moduleEntry);
				virtual ~AuthModule();

				int logonUser(char* username, char* domain, char* password);

				static pRdsAuthModuleEntry loadModuleEntry(const char* filename);
				static AuthModule* loadFromFileName(const char* fileName);
				static AuthModule* loadFromName(const char* name);

			private:
				rdsAuthModule* mAuth;
				pRdsAuthModuleEntry mModuleEntry;
				RDS_AUTH_MODULE_ENTRY_POINTS mEntryPoints;
			};
		}
	}
}

namespace moduleNS = freerds::sessionmanager::module;

#endif /* AUTH_MODULE_H_ */
