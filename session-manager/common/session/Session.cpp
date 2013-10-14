/**
 * Session store class
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

#include "Session.h"


namespace freerds{
	namespace sessionmanager{
		namespace session{

			Session::Session(long sessionID):mSessionID(sessionID) {

			}

			Session::~Session() {

			}

			std::string Session::getGroup() {
				return mGroupname;
			}

			void Session::setGroup(std::string groupname) {
				mGroupname = groupname;
			}

			std::string Session::getUserName() {
				return mUsername;
			}

			void Session::setUserName(std::string username) {
				mUsername = username;
			}

			long Session::getSessionID() {
				return mSessionID;
			}

		}
	}
}
