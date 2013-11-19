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
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <utils/StringHelpers.h>
#include <boost/lexical_cast.hpp>

namespace freerds
{
	namespace sessionmanager
	{
		namespace config
		{
			static wLog* logger_PropertyManager = WLog_Get("freerds.SessionManager.config.propertymanager");

			PropertyManager::PropertyManager()
			{

			};

			PropertyManager::~PropertyManager()
			{

			};

			bool PropertyManager::getPropertyInternal(long sessionID, std::string path, PROPERTY_STORE_HELPER & helper, std::string username) {
				// first try to resolve the sessionID
				bool found = false;
				std::string currentUserName;
				if (sessionID == 0) {
					// for no session, use username if its present
					if (username.size() == 0) {
						return -1;
					}
					currentUserName = username;
				} else {
					// for a given sessionID we try to get the username from the sessionstore
					sessionNS::Session * session = APP_CONTEXT.getSessionStore()->getSession(sessionID);
					if (session == NULL) {
						return -1;
					}
					currentUserName = session->getUserName();
				}

				if (mPropertyUserMap.find(currentUserName) != mPropertyUserMap.end()) {
					TPropertyMap * uPropMap = mPropertyUserMap[currentUserName];
					if (uPropMap->find(path) != uPropMap->end()) {
						// we found the setting ...
						helper = (*uPropMap)[path];
						return true;
					}
				}
				if ((!found) && (mPropertyGlobalMap.find(path) != mPropertyGlobalMap.end())) {
					helper = mPropertyGlobalMap[path];
					return true;
				}
				return false;
			}


			bool PropertyManager::getPropertyBool(long sessionID, std::string path, bool &value, std::string username)
			{
				PROPERTY_STORE_HELPER internStore;

				if (!getPropertyInternal(sessionID,path,internStore,username)) {
					return false;
				}

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

			bool PropertyManager::getPropertyNumber(long sessionID, std::string path, long &value, std::string username)
			{
				PROPERTY_STORE_HELPER internStore;

				if (!getPropertyInternal(sessionID,path,internStore,username)) {
					return false;
				}

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

			bool PropertyManager::getPropertyString(long sessionID, std::string path, std::string &value, std::string username)
			{
				PROPERTY_STORE_HELPER internStore;

				if (!getPropertyInternal(sessionID,path,internStore,username)) {
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


			int PropertyManager::setPropertyInternal(PROPERTY_LEVEL level, long sessionID,
					std::string path ,PROPERTY_STORE_HELPER helper, std::string username)
			{

				if (level == User) {
					std::string currentUserName;
					if (sessionID == 0) {
						// for no session, use username if its present
						if (username.size() == 0) {
							return -1;
						}
						currentUserName = username;
					} else {
						// for a given sessionID we try to get the username from the sessionstore
						sessionNS::Session * session = APP_CONTEXT.getSessionStore()->getSession(sessionID);
						if (session == NULL) {
							return -1;
						}
						currentUserName = session->getUserName();
					}
					// we have the username now
					if (mPropertyUserMap.find(currentUserName) != mPropertyUserMap.end()) {
						TPropertyMap * uPropMap = mPropertyUserMap[currentUserName];
						uPropMap->insert(std::make_pair(path, helper));
					} else {
						TPropertyMap * uPropMap = new TPropertyMap();
						uPropMap->insert(std::make_pair(path, helper));
						mPropertyUserMap.insert(std::make_pair(currentUserName, uPropMap));
					}
					return 0;
				} else if (level == Global) {
					mPropertyGlobalMap.insert(std::make_pair(path, helper));
					return 0;
				}
				return -1;
			}


			int PropertyManager::setPropertyBool(PROPERTY_LEVEL level, long sessionID,
					std::string path, bool value,std::string username)
			{

				PROPERTY_STORE_HELPER helper;
				helper.type = BoolType;
				helper.boolValue = value;

				return setPropertyInternal(level,sessionID,path,helper,username);
			}

			int PropertyManager::setPropertyNumber(PROPERTY_LEVEL level, long sessionID,
					std::string path, long value,std::string username)
			{
				// only global config for now
				PROPERTY_STORE_HELPER helper;
				helper.type = NumberType;
				helper.numberValue = value;

				return setPropertyInternal(level,sessionID,path,helper,username);
			}

			int PropertyManager::setPropertyString(PROPERTY_LEVEL level, long sessionID,
					std::string path, std::string value,std::string username)
			{
				// only global config for now
				PROPERTY_STORE_HELPER helper;
				helper.type = StringType;
				helper.stringValue = value;

				return setPropertyInternal(level,sessionID,path,helper,username);
			}



			int PropertyManager::parsePropertyGlobal(std::string parentPath, const boost::property_tree::ptree& tree,PROPERTY_LEVEL level) {

				BOOST_FOREACH(boost::property_tree::ptree::value_type const &v, tree)
				{
					boost::property_tree::ptree subtree = v.second;
					//assert(v.first.empty()); // array elements have no names
					if (v.second.data().size() != 0) {
						std::string fullPath = parentPath + "." + v.first;

						if (std::stringEndsWith(fullPath,"_number")) {
							std::string propertyName = fullPath.substr(0,fullPath.size() - strlen("_number"));
	    					std::replace(propertyName.begin(), propertyName.end(),'_','.');
							try {
								long test = boost::lexical_cast<long>(v.second.data());
								setPropertyNumber(level,0,propertyName,12);
							} catch (boost::bad_lexical_cast &){
								WLog_Print(logger_PropertyManager, WLOG_ERROR, "Could not cast %s to a number, property % ignored!",v.second.data().c_str(),propertyName.c_str());
							}
						} else if (std::stringEndsWith(fullPath,"_string")) {
							std::string propertyName = fullPath.substr(0,fullPath.size() - strlen("_string"));
							setPropertyString(level,0,propertyName,v.second.data());
						} else if (std::stringEndsWith(fullPath,"_bool")) {
							std::string propertyName = fullPath.substr(0,fullPath.size() - strlen("_number"));
							try {
								setPropertyBool(level,0,propertyName,boost::lexical_cast<bool>(v.second.data()));
							} catch (boost::bad_lexical_cast &){
								WLog_Print(logger_PropertyManager, WLOG_ERROR, "Could not cast %s to a bool, property % ignored!",v.second.data().c_str(), propertyName.c_str());
							}
						}
					}
					if (parentPath.size() == 0) {
						parsePropertyGlobal(v.first ,subtree,level);
					} else {
						parsePropertyGlobal(parentPath +"."+ v.first ,subtree,level);
					}
				}
			}

			int PropertyManager::loadProperties(std::string filename) {
				boost::property_tree::ptree pt;
				try {
					//boost::property_tree::read_json(filename, pt);
					boost::property_tree::read_ini(filename, pt);
					//boost::property_tree::read_xml(filename, pt);
					parsePropertyGlobal("",pt.get_child("global"),Global);
				} catch (boost::property_tree::file_parser_error & e) {
					WLog_Print(logger_PropertyManager, WLOG_ERROR, "Could not parse config file %s",filename.c_str());
				}

			}

			int PropertyManager::saveProperties(std::string filename) {
	            // Create an empty property tree object
	            using boost::property_tree::ptree;
	            ptree pt;

                ptree & node = pt.add("global","");

                TPropertyMap::iterator iter;

				for(iter = mPropertyGlobalMap.begin(); iter != mPropertyGlobalMap.end();iter++) {
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

                for(iterPPMap = mPropertyUserMap.begin(); iterPPMap != mPropertyUserMap.end();iterPPMap++) {
                	TPropertyMap * uPropMap = (TPropertyMap *)iterPPMap->second;
                	ptree & node1 = pt.add("user_" + iterPPMap->first, "");


    				for(iter = uPropMap->begin(); iter != uPropMap->end();iter++) {
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


				//write_json(filename + ".json",pt);
                write_ini(filename,pt);
                //boost::property_tree::xml_writer_settings<char> settings('\t', 1);
                //write_xml(filename+".xml",pt,std::locale(), settings);
				return 0;
			}

		}
	}
}
