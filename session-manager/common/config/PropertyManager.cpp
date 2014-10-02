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
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <utils/StringHelpers.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace config
		{
			static wLog* logger_PropertyManager = WLog_Get("freerds.SessionManager.config.propertymanager");

			std::string g_ConnectionPrefix = "current.connection.";

			PropertyManager::PropertyManager()
			{

			};

			PropertyManager::~PropertyManager()
			{

			};

			bool PropertyManager::getPropertyInternal(std::string path, PROPERTY_STORE_HELPER & helper)
			{
				if ((mPropertyGlobalMap.find(path) != mPropertyGlobalMap.end()))
				{
					helper = mPropertyGlobalMap[path];
					return true;
				}

				return false;
			}

			BOOL PropertyManager::getPropertyBool(std::string path, BOOL* value)
			{
				PROPERTY_STORE_HELPER internStore;

				if (!getPropertyInternal(path, internStore)) {
					return FALSE;
				}

				if (internStore.type != BoolType)
				{
					return FALSE;
				}
				else
				{
					*value = (BOOL) internStore.boolValue;
					return TRUE;
				}
			}

			bool PropertyManager::getPropertyBool(std::string path, bool &value)
			{
				PROPERTY_STORE_HELPER internStore;

				if (!getPropertyInternal(path, internStore)) {
					return FALSE;
				}

				if (internStore.type != BoolType)
				{
					return FALSE;
				}
				else
				{
					value = internStore.boolValue;
					return TRUE;
				}
			}

			BOOL PropertyManager::getPropertyNumber(std::string path, long* value)
			{
				PROPERTY_STORE_HELPER internStore;

				if (!getPropertyInternal(path, internStore)) {
					return false;
				}

				if (internStore.type != NumberType)
				{
					return false;
				}
				else
				{
					*value = internStore.numberValue;
					return true;
				}
			}

			BOOL PropertyManager::getPropertyString(std::string path, std::string &value)
			{
				PROPERTY_STORE_HELPER internStore;

				if (!getPropertyInternal(path, internStore)) {
					return false;
				}

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

			int PropertyManager::setPropertyInternal(std::string path, PROPERTY_STORE_HELPER helper)
			{
				mPropertyGlobalMap[path]= helper;
				return 0;
			}

			int PropertyManager::setPropertyBool(std::string path, bool value)
			{
				PROPERTY_STORE_HELPER helper;
				helper.type = BoolType;
				helper.boolValue = value;

				return setPropertyInternal(path, helper);
			}

			int PropertyManager::setPropertyNumber(std::string path, long value)
			{
				PROPERTY_STORE_HELPER helper;
				helper.type = NumberType;
				helper.numberValue = value;

				return setPropertyInternal(path, helper);
			}

			int PropertyManager::setPropertyString(std::string path, std::string value)
			{
				PROPERTY_STORE_HELPER helper;
				helper.type = StringType;
				helper.stringValue = value;

				return setPropertyInternal(path, helper);
			}

			int PropertyManager::parsePropertyGlobal(std::string parentPath, const boost::property_tree::ptree& tree)
			{
				bool useParentPath = false;

				if (parentPath.size() != 0)
				{
					if (!stringStartsWith(parentPath, "global"))
					{
						useParentPath = true;
					}
				}

				BOOST_FOREACH(boost::property_tree::ptree::value_type const &v, tree)
				{
					boost::property_tree::ptree subtree = v.second;

					if (v.second.data().size() != 0)
					{
						std::string fullPath;

						if (useParentPath) {
							fullPath = parentPath + "." + v.first;
						} else {
							fullPath =v.first;
						}

						if (std::stringEndsWith(fullPath,"_number"))
						{
							std::string propertyName = fullPath.substr(0,fullPath.size() - strlen("_number"));
							std::replace(propertyName.begin(), propertyName.end(),'_','.');
							try {
								long number = boost::lexical_cast<long>(v.second.data());
								setPropertyNumber(propertyName, number);
							} catch (boost::bad_lexical_cast &){
								WLog_Print(logger_PropertyManager, WLOG_ERROR, "Could not cast %s to a number, property % ignored!",v.second.data().c_str(),propertyName.c_str());
							}
						}
						else if (std::stringEndsWith(fullPath,"_string"))
						{
							std::string propertyName = fullPath.substr(0,fullPath.size() - strlen("_string"));
							std::replace(propertyName.begin(), propertyName.end(),'_','.');
							setPropertyString(propertyName, v.second.data());
						}
						else if (std::stringEndsWith(fullPath,"_bool"))
						{
							std::string propertyName = fullPath.substr(0,fullPath.size() - strlen("_bool"));
							std::replace(propertyName.begin(), propertyName.end(),'_','.');
							try {
								setPropertyBool(propertyName,boost::lexical_cast<bool>(v.second.data()));
							} catch (boost::bad_lexical_cast &){
								WLog_Print(logger_PropertyManager, WLOG_ERROR, "Could not cast %s to a bool, property %s ignored!",v.second.data().c_str(), propertyName.c_str());
							}
						}
					}

					if (parentPath.size() == 0) {
						parsePropertyGlobal(v.first, subtree);
					} else {
						parsePropertyGlobal(parentPath + "." + v.first, subtree);
					}
				}

				return 0;
			}

			int PropertyManager::loadProperties(std::string filename)
			{
				boost::property_tree::ptree pt;

				try {
					boost::property_tree::read_ini(filename, pt);
					parsePropertyGlobal("", pt);
				} catch (boost::property_tree::file_parser_error & e) {
					WLog_Print(logger_PropertyManager, WLOG_ERROR, "Could not parse config file %s",filename.c_str());
				}

				return 0;
			}

			int PropertyManager::saveProperties(std::string filename)
			{
				using boost::property_tree::ptree;
				ptree pt;

				ptree & node = pt.add("global","");

				TPropertyMap::iterator iter;

				for (iter = mPropertyGlobalMap.begin(); iter != mPropertyGlobalMap.end(); iter++)
				{
					std::string corrected = iter->first;
					std::replace(corrected.begin(), corrected.end(),'.','_');

					if (iter->second.type == BoolType) {
						node.add(corrected+"_bool",iter->second.boolValue);
					} else if (iter->second.type == StringType) {
						node.add(corrected + "_string",iter->second.stringValue);
					} else if (iter->second.type == NumberType) {
						node.add(corrected+"_number",iter->second.numberValue);
					}
				}

				TPropertyPropertyMap::iterator iterPPMap;

				for (iterPPMap = mPropertyUserMap.begin(); iterPPMap != mPropertyUserMap.end(); iterPPMap++)
				{
					TPropertyMap* uPropMap = (TPropertyMap*) iterPPMap->second;
					ptree & node1 = pt.add("user_" + iterPPMap->first, "");

					for (iter = uPropMap->begin(); iter != uPropMap->end(); iter++)
					{
						std::string corrected = iter->first;
						std::replace(corrected.begin(), corrected.end(),'.','_');

						if (iter->second.type == BoolType) {
							node1.add(corrected+"_bool",iter->second.boolValue);
						} else if (iter->second.type == StringType) {
							node1.add(corrected + "_string",iter->second.stringValue);
						} else if (iter->second.type == NumberType) {
							node1.add(corrected+"_number",iter->second.numberValue);
						}
					}
				}

				write_ini(filename,pt);

				return 0;
			}
		}
	}
}
