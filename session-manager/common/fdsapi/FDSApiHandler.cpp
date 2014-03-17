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
#include "FDSApiHandler.h"
#include <session/SessionStore.h>
#include <appcontext/ApplicationContext.h>

#include <winpr/thread.h>
#include <winpr/synch.h>
#include <winpr/wlog.h>
#include <winpr/wtsapi.h>

#include <call/CallOutFdsApiVirtualChannelOpen.h>


namespace freerds{
	namespace sessionmanager{
		namespace fdsapi {

		static wLog * logger_FDSApiHandler = WLog_Get("freerds.sessionmanager.fdsapihandler");


		FDSApiHandler::FDSApiHandler()
		{
		}

		FDSApiHandler::~FDSApiHandler()
		{
		}

		TINT32 FDSApiHandler::ping(const TINT32 input)
		{
		}

		TINT32 FDSApiHandler::authenticateUser(
			const TSTRING& authToken,
			const TINT32 sessionId,
			const TSTRING& username,
			const TSTRING& password,
			const TSTRING& domain)
		{
			TINT32 authStatus;

			authStatus = APP_CONTEXT.authenticateUser(
				sessionId,
				username,
				password,
				domain);

			return authStatus;
		}

		void FDSApiHandler::virtualChannelOpen(
			TSTRING& _return,
			const TSTRING& authToken,
			const TINT32 sessionId,
			const TSTRING& virtualName)
		{
			// ....

			callNS::CallOutFdsApiVirtualChannelOpen openCall;
			openCall.setSessionID(sessionId);
			openCall.setVirtualName(virtualName);

			APP_CONTEXT.getRpcOutgoingQueue()->addElement(&openCall);
			WaitForSingleObject(openCall.getAnswerHandle(),INFINITE);
			if (openCall.getResult() == 0)
			{
				// no error
				_return = openCall.getConnectionString();
			}
			else
			{
				// report error
				_return = "";
			}

		}

		void FDSApiHandler::virtualChannelOpenEx(
			TSTRING& _return,
			const TSTRING& authToken,
			const TINT32 sessionId,
			const TSTRING& virtualName,
			const INT32 flags)
		{
		}

		TBOOL FDSApiHandler::virtualChannelClose(
			const TSTRING& authToken,
			const TINT32 sessionId,
			const TSTRING& virtualName)
		{
			return false;
		}

		TBOOL FDSApiHandler::disconnectSession(
			const TSTRING& authToken,
			const TINT32 sessionId,
			const TBOOL wait)
		{
			sessionNS::SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
			sessionNS::SessionPtr session = sessionStore->getSession(sessionId);

			if (session)
			{
				/* TODO: Disconnect the session. */
			}

			return false;
		}

		TBOOL FDSApiHandler::logoffSession(
			const TSTRING& authToken,
			const TINT32 sessionId,
			const TBOOL wait)
		{
			sessionNS::SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
			sessionNS::SessionPtr session = sessionStore->getSession(sessionId);

			if (session)
			{
				/* TODO: Logoff the session. */
			}

			return false;
		}

		TBOOL FDSApiHandler::shutdownSystem(
			const TSTRING& authToken,
			const TINT32 shutdownFlag)
		{
			/* TODO: Shutdown the system. */

			return false;
		}

		void FDSApiHandler::enumerateSessions(
			TReturnEnumerateSessions& _return,
			const TSTRING& authToken,
			const TINT32 Version)
		{
			DWORD count;
			DWORD index;

			sessionNS::SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
			std::list<sessionNS::SessionPtr> sessions = sessionStore->getAllSessions();

			count = sessions.size();
			TSessionInfoList list(count);

			std::list<sessionNS::SessionPtr>::iterator session = sessions.begin();

			for (index = 0; index < count; index++)
			{
				list.at(index).sessionId = (*session)->getSessionID();
				list.at(index).winStationName = (*session)->getWinStationName();
				list.at(index).connectState = WTSActive;
				session++;
			}

			_return.sessionInfoList = list;

			_return.returnValue = true;
		}

		void FDSApiHandler::getSessionInformation(
			TReturnGetSessionInformation& _return,
			const TSTRING& authToken,
			const TINT32 sessionId,
			const TINT32 infoClass)
		{
			sessionNS::SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
			sessionNS::SessionPtr session = sessionStore->getSession(sessionId);

			if (session)
			{
				_return.returnValue = true;

				switch (infoClass)
				{
					case WTSSessionId:
						_return.infoValue.int32Value = session->getSessionID();
						break;

					case WTSUserName:
						_return.infoValue.stringValue = session->getUserName();
						break;

					case WTSWinStationName:
						_return.infoValue.stringValue = session->getWinStationName();
						break;

					case WTSDomainName:
						_return.infoValue.stringValue = session->getDomain();
						break;

					case WTSConnectState:
						_return.infoValue.int32Value = session->getConnectState();
						break;

					case WTSClientBuildNumber:
						_return.infoValue.int32Value = session->getClientBuildNumber();
						break;

					case WTSClientName:
						_return.infoValue.stringValue = session->getClientName();
						break;

					case WTSClientProductId:
						_return.infoValue.int16Value = session->getClientProductId();
						break;

					case WTSClientHardwareId:
						_return.infoValue.int32Value = session->getClientHardwareId();
						break;

					case WTSClientAddress:
						_return.infoValue.stringValue = session->getClientAddress();
						break;

					case WTSClientDisplay:
						_return.infoValue.displayValue.displayWidth = session->getClientDisplayWidth();
						_return.infoValue.displayValue.displayHeight = session->getClientDisplayHeight();
						_return.infoValue.displayValue.colorDepth = session->getClientDisplayColorDepth();
						break;

					case WTSClientProtocolType:
						_return.infoValue.int16Value = session->getClientProtocolType();
						break;

					default:
						_return.returnValue = false;
						break;
				}
			}
			else
			{
				_return.returnValue = false;
			}
		}

		TBOOL FDSApiHandler::setSessionInformation(
			const TSTRING& authToken,
			const TINT32 sessionId,
			const TINT32 infoClass,
			const TSessionInfoValue& infoValue)
		{
			return false;
		}

		}
	}
}


