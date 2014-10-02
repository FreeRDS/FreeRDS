/**
 * Property Manager to store and receive the Config
 *
 * there will be 3 kinds of propertys
 * PropertyBool
 * PropertyString
 * PropertyNumber
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

#include "PropertyManager.h"

#include <winpr/wlog.h>

#include <appcontext/ApplicationContext.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace config
		{
			PropertyManager::PropertyManager()
			{
				ini = IniFile_New();
			};

			PropertyManager::~PropertyManager()
			{
				IniFile_Free(ini);
			};

			BOOL PropertyManager::getPropertyBool(std::string path, BOOL* value)
			{
				int intVal;

				if (!ini)
					return FALSE;

				intVal = IniFile_GetKeyValueInt(ini, "global", path.c_str());

				if (intVal < 0)
					return FALSE;

				*value = (!intVal) ? FALSE : TRUE;

				return TRUE;
			}

			BOOL PropertyManager::getPropertyBool(std::string path, bool &value)
			{
				int intVal;

				if (!ini)
					return FALSE;

				intVal = IniFile_GetKeyValueInt(ini, "global", path.c_str());

				if (intVal < 0)
					return FALSE;

				value = (!intVal) ? false : true;

				return TRUE;
			}

			BOOL PropertyManager::getPropertyNumber(std::string path, long* value)
			{
				int intVal;

				if (!ini)
					return FALSE;

				intVal = IniFile_GetKeyValueInt(ini, "global", path.c_str());

				if (intVal < 0)
					return FALSE;

				*value = intVal;

				return TRUE;
			}

			BOOL PropertyManager::getPropertyString(std::string path, std::string &value)
			{
				const char* strVal;

				if (!ini)
					return FALSE;

				strVal = IniFile_GetKeyValueString(ini, "global", path.c_str());

				if (!strVal)
					return FALSE;

				value = strVal;

				return TRUE;
			}

			int PropertyManager::setPropertyBool(std::string path, bool value)
			{
				if (!ini)
					return 0;

				IniFile_SetKeyValueInt(ini, "global", path.c_str(), value ? 1 : 0);

				return 0;
			}

			int PropertyManager::setPropertyNumber(std::string path, long value)
			{
				if (!ini)
					return 0;

				IniFile_SetKeyValueInt(ini, "global", path.c_str(), value);

				return 0;
			}

			int PropertyManager::setPropertyString(std::string path, std::string value)
			{
				int status;

				if (!ini)
					return 0;

				status = IniFile_SetKeyValueString(ini, "global", path.c_str(), value.c_str());

				return 0;
			}

			int PropertyManager::loadProperties(std::string filename)
			{
				if (IniFile_ReadFile(ini, filename.c_str()) < 0)
					return 0;

				return 0;
			}

			int PropertyManager::saveProperties(std::string filename)
			{
				if (!ini)
					return 0;

				return 0;
			}
		}
	}
}
