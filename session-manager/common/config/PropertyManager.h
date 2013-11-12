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


#ifndef PROPERTYMANAGER_H_
#define PROPERTYMANAGER_H_

#include <string>
#include <map>
#include "PropertyLevel.h"

namespace freerds
{
	namespace sessionmanager
	{
		namespace config
		{
			typedef enum _PROPERTY_STORE_TYPE
			{
				BoolType = 1,
				NumberType = 2,
				StringType = 3
			} PROPERTY_STORE_TYPE, *PPROPERTY_STORE_TYPE;

			typedef struct _PROPERTY_STORE_HELPER
			{
				PROPERTY_STORE_TYPE type;
				bool boolValue;
				long numberValue;
				std::string stringValue;
			} PROPERTY_STORE_HELPER, *PPROPERTY_STORE_HELPER;

			typedef std::map<std::string, PROPERTY_STORE_HELPER> TPropertyMap;
			typedef std::pair<std::string, PROPERTY_STORE_HELPER> TPropertyPair;

			typedef std::map<std::string, TPropertyMap> TPropertyPropertyMap;
			typedef std::pair<std::string, TPropertyMap> TPropertyPropertyPair;

			class PropertyManager
			{
			public:
				PropertyManager();
				~PropertyManager();

				bool getPropertyBool(long sessionID, std::string path, bool &value);
				bool getPropertyNumber(long sessionID, std::string path, long &value);
				bool getPropertyString(long sessionID, std::string path, std::string &value);

				int setPropertyBool(PROPERTY_LEVEL level, long sessionID, std::string path, bool value);
				int setPropertyNumber(PROPERTY_LEVEL level, long sessionID, std::string path, long value);
				int setPropertyString(PROPERTY_LEVEL level, long sessionID, std::string path, std::string value);

				int saveProperties();
				int loadProperties();

			private:
				TPropertyMap mPropertyGlobalMap;
				TPropertyPropertyMap mPropertyGroupMap;
				TPropertyPropertyMap mPropertyUserMap;
			};
		}
	}
}

namespace configNS = freerds::sessionmanager::config;

#endif /* PROPERTYMANAGER_H_ */
