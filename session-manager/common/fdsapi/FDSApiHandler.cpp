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
#include <call/TaskSwitchTo.h>


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
			TINT32 authStatus = -1;

			configNS::PropertyManager* propertyManager = APP_CONTEXT.getPropertyManager();
			sessionNS::ConnectionStore* connectionStore = APP_CONTEXT.getConnectionStore();
			sessionNS::SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
			sessionNS::SessionPtr authSession = sessionStore->getSession(sessionId);

			// Check to see if the session is valid.
			if (!authSession || !authSession->isAuthSession())
			{
				WLog_Print(logger_FDSApiHandler, WLOG_ERROR,
					"Session id %d is not a valid session",
					sessionId);
				return -1;
			}

			// Check for an associated connection.
			long connectionId = connectionStore->getConnectionIdForSessionId(sessionId);
			sessionNS::ConnectionPtr connection = connectionStore->getConnection(connectionId);
			if (!connection)
			{
				WLog_Print(logger_FDSApiHandler, WLOG_ERROR,
					"Session id %d has no associated connection",
					sessionId);
				return -1;
			}

			// Attempt to authenticate the user.
			authStatus = connection->authenticateUser(username, domain, password);
			if (authStatus != 0)
			{
				WLog_Print(logger_FDSApiHandler, WLOG_ERROR,
					"Could not authenticate session id %d user '%s' in domain '%s'",
					sessionId,
					username.c_str(),
					domain.c_str());
				return authStatus;
			}

			// The user has been authenticated.  Now we can either 1) connect
			// to a disconnected session or 2) create a new user session.

			sessionNS::SessionPtr userSession;
			bool isReconnectAllowed;
			bool isNewSession;

			if (!propertyManager->getPropertyBool(0, "session.reconnect", isReconnectAllowed, username))
			{
				isReconnectAllowed = true;
			}

			if (isReconnectAllowed)
			{
				userSession = sessionStore->getFirstDisconnectedSessionUserName(username, domain);
			}

			if (userSession && (userSession->getConnectState() == WTSDisconnected))
			{
				// Connect to the disconnected session.
				isNewSession = false;
			}
			else
			{
				// Create a new session.
				userSession = sessionStore->createSession();
				if (!userSession)
				{
					WLog_Print(logger_FDSApiHandler, WLOG_ERROR,
						"Cannot create a new session for user '%s' in domain '%s'",
						username.c_str(),
						domain.c_str());
					return -1;
				}

				isNewSession = true;
			}

			// Switch to the user session.
			userSession->setUserName(username);
			userSession->setDomain(domain);
			userSession->setClientDisplayWidth(authSession->getClientDisplayWidth());
			userSession->setClientDisplayHeight(authSession->getClientDisplayHeight());
			userSession->setClientDisplayColorDepth(authSession->getClientDisplayColorDepth());
			userSession->setClientName(authSession->getClientName());
			userSession->setClientAddress(authSession->getClientAddress());
			userSession->setClientBuildNumber(authSession->getClientBuildNumber());
			userSession->setClientProductId(authSession->getClientProductId());
			userSession->setClientHardwareId(authSession->getClientHardwareId());
			userSession->setClientProtocolType(authSession->getClientProtocolType());
			userSession->setWinStationName(authSession->getWinStationName());

			userSession->generateUserToken();

			userSession->generateEnvBlockAndModify();

			std::string moduleConfigName;

			if (!propertyManager->getPropertyString(sessionId, "module", moduleConfigName))
			{
				moduleConfigName = "X11";
			}
			userSession->setModuleConfigName(moduleConfigName);

			connection->setSessionId(userSession->getSessionID());

			connection->getClientInformation()->with = userSession->getClientDisplayWidth();
			connection->getClientInformation()->height = userSession->getClientDisplayHeight();
			connection->getClientInformation()->colordepth = userSession->getClientDisplayColorDepth();

			if (isNewSession)
			{
				WLog_Print(logger_FDSApiHandler, WLOG_INFO,
					"Starting new session %d for user '%s' in domain '%s'",
					sessionId,
					username.c_str(),
					domain.c_str());

				std::string pipeName;
				if (!userSession->startModule(pipeName))
				{
					WLog_Print(logger_FDSApiHandler, WLOG_ERROR,
						"ModuleConfig %s does not start properly for user '%s' in domain '%s'",
						moduleConfigName.c_str(),
						username.c_str(),
						domain.c_str());
					return -1;
				}
			}

			callNS::TaskSwitchToPtr switchToTask;
			switchToTask = callNS::TaskSwitchToPtr(new callNS::TaskSwitchTo());
			switchToTask->setConnectionId(connectionId);
			switchToTask->setServiceEndpoint(userSession->getPipeName());
			switchToTask->setOldSessionId(sessionId);
			switchToTask->setNewSessionId(userSession->getSessionID());
			APP_CONTEXT.addTask(switchToTask);

			// The user session is now active.
			userSession->setConnectState(WTSActive);

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

		void FDSApiHandler::querySessionInformation(
			TReturnQuerySessionInformation& _return,
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

		}
	}
}


