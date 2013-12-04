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

			virtual TDWORD ping(const TDWORD input);
			virtual void virtualChannelOpen(TLPSTR& _return, const TLPSTR& authToken, const TDWORD sessionId, const TLPSTR& virtualName);
			virtual void virtualChannelOpenEx(TLPSTR& _return, const TLPSTR& authToken, const TDWORD sessionId, const TLPSTR& virtualName, const TDWORD flags);
			virtual bool virtualChannelClose(const TLPSTR& authToken, const TDWORD sessionId, const TLPSTR& virtualName);
			virtual bool disconnectSession(const TLPSTR& authToken, const TDWORD sessionId, const bool wait);
			virtual bool logoffSession(const TLPSTR& authToken, const TDWORD sessionId, const bool wait);
			virtual void enumerateSessions(ReturnEnumerateSession& _return, const TLPSTR& authToken, const TDWORD Version);
		};

		}
	}
}
namespace fdsapiNS = freerds::sessionmanager::fdsapi;

#endif /* FDSAPISHANDLER_H_ */
