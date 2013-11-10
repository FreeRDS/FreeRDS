/**
 * Property Manager to store and receive the Config
 *
 * there will be 3 kinds of propertys
 * PropertyBool
 * PropertyString
 * PropertyNumber
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
			static wLog * logger_PropertyManager = WLog_Get("freerds.sessionmanager.config.propertymanager");

			PropertyManager::PropertyManager()
			{

			};

			PropertyManager::~PropertyManager()
			{

			};

			bool PropertyManager::getPropertyBool(long sessionID, std::string path, bool &value)
			{
				// only global config for now
				if (mPropertyGlobalMap.find(path) != mPropertyGlobalMap.end())
				{
					PROPERTY_STORE_HELPER internStore = mPropertyGlobalMap[path];

					if (internStore.type != BoolType)
					{
						return false;
					}
					else
					{
						value = internStore.boolValue;
						return true;
					}
				}
				else
				{
					// element not found
					return false;
				}
			}

			bool PropertyManager::getPropertyNumber(long sessionID, std::string path, long &value)
			{
				// only global config for now
				if (mPropertyGlobalMap.find(path) != mPropertyGlobalMap.end())
				{
					PROPERTY_STORE_HELPER internStore = mPropertyGlobalMap[path];

					if (internStore.type != NumberType)
					{
						return false;
					}
					else
					{
						value = internStore.numberValue;
						return true;
					}
				}
				else
				{
					// element not found
					return false;
				}
			}

			bool PropertyManager::getPropertyString(long sessionID, std::string path, std::string &value)
			{
				// only global config for now
				if (mPropertyGlobalMap.find(path) != mPropertyGlobalMap.end())
				{
					PROPERTY_STORE_HELPER internStore = mPropertyGlobalMap[path];

					if (internStore.type != StringType)
					{
						return false;
					}
					else
					{
						value = internStore.stringValue;
						return true;
					}
				}
				else
				{
					// element not found
					return false;
				}
			}

			int PropertyManager::setPropertyBool(PROPERTY_LEVEL level, long sessionID,
					std::string path, bool value)
			{
				// only global config for now
				PROPERTY_STORE_HELPER helper;
				helper.type = BoolType;
				helper.boolValue = value;

				mPropertyGlobalMap.insert(std::make_pair(path,helper));
				return 0;
			}

			int PropertyManager::setPropertyNumber(PROPERTY_LEVEL level, long sessionID,
					std::string path, long value)
			{
				// only global config for now
				PROPERTY_STORE_HELPER helper;
				helper.type = NumberType;
				helper.numberValue = value;

				mPropertyGlobalMap.insert(std::make_pair(path,helper));
				return 0;
			}

			int PropertyManager::setPropertyString(PROPERTY_LEVEL level, long sessionID,
					std::string path, std::string value)
			{
				// only global config for now
				PROPERTY_STORE_HELPER helper;
				helper.type = StringType;
				helper.stringValue = value;

				mPropertyGlobalMap.insert(std::make_pair(path,helper));
				return 0;
			}

			int PropertyManager::loadProperties()
			{

			}

			int PropertyManager::saveProperties()
			{

			}
		}
	}
}
