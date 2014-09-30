/**
 *  FDSApiHandler
 *
 *  Starts and stops the thrift server.
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

#ifndef FDSAPISHANDLER_H_
#define FDSAPISHANDLER_H_

#include <fdsapi/fdsapi.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <boost/shared_ptr.hpp>
#include <thrift/server/TServer.h>

namespace freerds{
	namespace sessionmanager{
		namespace fdsapi {

		class FDSApiHandler : virtual public fdsapiIf {
		 public:
			FDSApiHandler();
			virtual ~FDSApiHandler();

			virtual TINT32 ping(const TINT32 input);

			virtual TINT32 authenticateUser(const TSTRING& authToken, const TINT32 sessionId, const TSTRING& username, const TSTRING& password, const TSTRING& domain);

			virtual void virtualChannelOpen(TSTRING& _return, const TSTRING& authToken, const TINT32 sessionId, const TSTRING& virtualName);
			virtual void virtualChannelOpenEx(TSTRING& _return, const TSTRING& authToken, const TINT32 sessionId, const TSTRING& virtualName, const TINT32 flags);
			virtual TBOOL virtualChannelClose(const TSTRING& authToken, const TINT32 sessionId, const TSTRING& virtualName);
			virtual TBOOL disconnectSession(const TSTRING& authToken, const TINT32 sessionId, const TBOOL wait);
			virtual TBOOL logoffSession(const TSTRING& authToken, const TINT32 sessionId, const TBOOL wait);
			virtual TBOOL shutdownSystem(const TSTRING& authToken, const TINT32 shutdownFlag);
			virtual void enumerateSessions(TReturnEnumerateSessions& _return, const TSTRING& authToken, const TINT32 Version);
			virtual void querySessionInformation(TReturnQuerySessionInformation& _return, const TSTRING& authToken, const TINT32 sessionId, const TINT32 infoClass);
		};

		}
	}
}
namespace fdsapiNS = freerds::sessionmanager::fdsapi;

#endif /* FDSAPISHANDLER_H_ */
