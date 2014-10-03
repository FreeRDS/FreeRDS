/**
 * Module Manager
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

#include "ModuleManager.h"

#include <winpr/crt.h>
#include <winpr/wlog.h>
#include <winpr/file.h>
#include <winpr/path.h>
#include <winpr/library.h>
#include <winpr/environment.h>

#include <config/PropertyCWrapper.h>
#include <utils/StringHelpers.h>

#include <vector>

#include <config/PropertyCWrapper.h>
#include "CallBacks.h"

namespace freerds
{
	namespace module
	{
		static wLog* logger_ModuleManager = WLog_Get("freerds.sessionmanager.module.modulemanager");

		ModuleManager::ModuleManager()
		{
			this->pathSeparator = PathGetSeparatorA(PATH_STYLE_NATIVE);
		}

		ModuleManager::~ModuleManager()
		{
			free(this->defaultModuleName);
		}

		int ModuleManager::loadModulesFromPathAndEnv(std::string path, std::string pattern)
		{
			// the the Environment variable
			DWORD nSize = GetEnvironmentVariableA(MODULE_ENV_VAR, NULL, 0);

			if (nSize)
			{
				// we found the env variable
				char* lpBuffer = (LPSTR) malloc(nSize);
				nSize = GetEnvironmentVariableA(MODULE_ENV_VAR, lpBuffer, nSize);

				WLog_Print(logger_ModuleManager, WLOG_TRACE,
						"found env variable %s with content %s", MODULE_ENV_VAR, lpBuffer);

				std::string envpath(lpBuffer);
				std::vector<std::string> pathList = split<std::string>(envpath, ":");

				for (int run = 0; run < pathList.size(); run++)
				{
					loadModulesFromPath(pathList[run], pattern);
				}
			}
			else
			{
				WLog_Print(logger_ModuleManager, WLOG_TRACE, "did not find env variable %s", MODULE_ENV_VAR);
			}

			loadModulesFromPath(path, pattern);

			return 0;
		}

		int ModuleManager::loadModulesFromPath(std::string path, std::string pattern)
		{
			 HANDLE hFind;
			 WIN32_FIND_DATA FindFileData;
			 std::string fullsearch = path + pathSeparator + pattern;

			 hFind = FindFirstFile(fullsearch.c_str(), &FindFileData);
			 WLog_Print(logger_ModuleManager, WLOG_TRACE, "scanning with in directory %s for modules", fullsearch.c_str());

			 if (hFind == INVALID_HANDLE_VALUE)
			 {
				 WLog_Print(logger_ModuleManager, WLOG_ERROR, "FindFirstFile (path = %s) failed", fullsearch.c_str());
				 return -1;
			 }

			 do
			 {
				 if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				 {
					 // do nothing
				 }
				 else
				 {
					 // try to add this module ...
					 addModule(path, std::string(FindFileData.cFileName));

				 }
			 }
			 while (FindNextFile(hFind, &FindFileData) != 0);
			 return 0;
		}

		int ModuleManager::addModule(std::string path, std::string modulename)
		{
			HMODULE hLib;
			int result = 0;
			pRdsModuleEntry entry;
			RDS_MODULE_ENTRY_POINTS entrypoints;

			std::string fullFileName = path + pathSeparator + modulename;

			WLog_Print(logger_ModuleManager, WLOG_TRACE, "loading library %s", fullFileName.c_str());

			hLib = LoadLibrary(fullFileName.c_str());

			if (!hLib)
			{
				WLog_Print(logger_ModuleManager, WLOG_ERROR, "loading library %s failed", fullFileName.c_str());
				return -1;
			}

			// get the exports
			entry = (pRdsModuleEntry) GetProcAddress(hLib, RDS_MODULE_ENTRY_POINT_NAME);

			if (entry)
			{
				ZeroMemory(&entrypoints, sizeof(RDS_MODULE_ENTRY_POINTS));

				entrypoints.config.getPropertyNumber = getPropertyNumber;
				entrypoints.config.getPropertyString = getPropertyString;
				entrypoints.status.shutdown = CallBacks::shutdown;
				result = entry(&entrypoints);

				if (result == 0)
				{
					// no error occurred
					Module* module = new Module();

					if (module->initModule(hLib, fullFileName, &entrypoints) == 0)
					{
						// check if module with same name is registered.
						if (mModulesMap.count(module->getName()))
						{
							WLog_Print(logger_ModuleManager, WLOG_INFO,
									"library %s loaded, but another library has already registered module name %s", module->getName().c_str());
							delete module;
							return -1;
						}
						else
						{
							WLog_Print(logger_ModuleManager, WLOG_INFO, "library %s loaded properly", fullFileName.c_str());
							// add this module to the map
							mModulesMap.insert(std::pair<std::string,Module *>(module->getName(), module));
						}
					}
					else
					{
						WLog_Print(logger_ModuleManager, WLOG_ERROR, "library %s not loaded", fullFileName.c_str());
						delete module;
						return -1;
					}
				}
				else
				{
					WLog_Print(logger_ModuleManager, WLOG_ERROR, "library %s function %s reported error %d", fullFileName.c_str(), RDS_MODULE_ENTRY_POINT_NAME, result);
					return -1;
				}
			}
			else
			{
				WLog_Print(logger_ModuleManager, WLOG_ERROR, "library %s does not export function %s", fullFileName.c_str(), RDS_MODULE_ENTRY_POINT_NAME);
				return -1;
			}
			return 0;
		}

		Module* ModuleManager::getModule(std::string moduleName)
		{
			if (mModulesMap.count(moduleName))
			{
				return mModulesMap[moduleName];
			}
			else
			{
				return NULL;
			}
		}
	}
}
