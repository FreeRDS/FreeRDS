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

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/thread.h>

#include <boost/shared_ptr.hpp>

#include <vector>
#include <iostream>

namespace freerds {

typedef std::vector<class TSessionInfo>  TSessionInfoList;

class TClientDisplay {
public:
	INT32 displayWidth;
	INT32 displayHeight;
	INT32 colorDepth;
};

class TSessionInfo {
public:
	INT32 sessionId;
	INT32 connectState;
	std::string winStationName;
};

class TSessionInfoValue {
public:
	bool boolValue;
	INT16 int16Value;
	INT32 int32Value;
	std::string stringValue;
	TClientDisplay displayValue;
};

class TReturnEnumerateSessions {
public:
	bool returnValue;
	TSessionInfoList sessionInfoList;
};

class TReturnQuerySessionInformation {
public:
	bool returnValue;
	TSessionInfoValue infoValue;
};

}

namespace freerds
{
	class FDSApiHandler
	{
	public:
		FDSApiHandler();
		virtual ~FDSApiHandler();

		virtual INT32 ping(const INT32 input);
		virtual INT32 authenticateUser(const std::string& authToken, const INT32 sessionId, const std::string& username, const std::string& password, const std::string& domain);
		virtual UINT32 virtualChannelOpen(std::string& _return, const std::string& authToken, const INT32 sessionId, const std::string& virtualName);
		virtual UINT32 virtualChannelOpenEx(std::string& _return, const std::string& authToken, const INT32 sessionId, const std::string& virtualName, const INT32 flags);
		virtual bool virtualChannelClose(const std::string& authToken, const INT32 sessionId, const std::string& virtualName);
		virtual bool disconnectSession(const std::string& authToken, const INT32 sessionId, const bool wait);
		virtual bool logoffSession(const std::string& authToken, const INT32 sessionId, const bool wait);
		virtual bool shutdownSystem(const std::string& authToken, const INT32 shutdownFlag);
		virtual void enumerateSessions(TReturnEnumerateSessions& _return, const std::string& authToken, const INT32 Version);
		virtual void querySessionInformation(TReturnQuerySessionInformation& _return, const std::string& authToken, const INT32 sessionId, const INT32 infoClass);
	};
}

#endif /* FDSAPISHANDLER_H_ */
