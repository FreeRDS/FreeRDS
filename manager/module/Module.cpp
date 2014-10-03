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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Module.h"
#include <winpr/wlog.h>
#include <winpr/library.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace module
		{
			static wLog* logger_Module = WLog_Get("freerds.SessionManager.module.module");

			Module::Module() : mfpNew(0), mfpFree(0)
				,mfpStart(0), mfpStop(0)
			{

			}

			int Module::initModule(HMODULE libHandle, std::string moduleFileName, RDS_MODULE_ENTRY_POINTS* entrypoints)
			{
				char buffer[260];

				if (!entrypoints)
					return -1;

				mModuleFile = moduleFileName;

				if ((!entrypoints->Free) || (!entrypoints->New)|| (!entrypoints->Start)|| (!entrypoints->Stop))
				{
					WLog_Print(logger_Module, WLOG_ERROR, "not all passed function pointers are set for module %s",mModuleFile.c_str());
					return -1;
				}

				if (!entrypoints->Name)
				{
					 WLog_Print(logger_Module, WLOG_ERROR, "no ModuleName is set for module %s",mModuleFile.c_str());
					 return -1;
				}

				mfpFree = entrypoints->Free;
				mfpNew = entrypoints->New;
				mfpStart = entrypoints->Start;
				mfpStop = entrypoints->Stop;
				mModuleName = std::string(entrypoints->Name);

				return 0;
			}

			Module::~Module()
			{

			}

			std::string Module::getName()
			{
				return mModuleName;
			}

			RDS_MODULE_COMMON* Module::newContext()
			{
				return mfpNew();
			}

			void Module::freeContext(RDS_MODULE_COMMON* context)
			{
				return mfpFree(context);
			}

			std::string Module::start(RDS_MODULE_COMMON* context)
			{
				char * pipeName;
				std::string pipeNameStr;
				pipeName = mfpStart(context);

				if (pipeName)
					pipeNameStr.assign(pipeName);

				return pipeNameStr;
			}

			int Module::stop(RDS_MODULE_COMMON* context)
			{
				return mfpStop(context);
			}
		}
	}
}

