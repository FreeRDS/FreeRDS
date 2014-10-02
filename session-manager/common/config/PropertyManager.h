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


#ifndef PROPERTYMANAGER_H_
#define PROPERTYMANAGER_H_

#include <winpr/crt.h>
#include <winpr/ini.h>

#include <string>

namespace freerds
{
	namespace sessionmanager
	{
		namespace config
		{
			class PropertyManager {
			public:
				PropertyManager();
				~PropertyManager();

				BOOL getPropertyBool(std::string path, BOOL* value);
				BOOL getPropertyBool(std::string path, bool &value);
				BOOL getPropertyNumber(std::string path, long* value);
				BOOL getPropertyString(std::string path, std::string &value);

				int setPropertyBool(std::string path, bool value);
				int setPropertyNumber(std::string path, long value);
				int setPropertyString(std::string path, std::string value);

				int saveProperties(std::string filename);
				int loadProperties(std::string filename);

			private:
				wIniFile* ini;
			};
		}
	}
}
namespace configNS = freerds::sessionmanager::config;

#endif /* PROPERTYMANAGER_H_ */
