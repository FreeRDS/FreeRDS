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
#include <session/ApplicationContext.h>

#include <winpr/thread.h>
#include <winpr/synch.h>
#include <winpr/wlog.h>
#include <winpr/wtsapi.h>

#include <call/CallOutVirtualChannelOpen.h>
#include <call/TaskSwitchTo.h>

namespace freerds
{
	static wLog* logger_FDSApiHandler = WLog_Get("freerds.fdsapihandler");

	FDSApiHandler::FDSApiHandler()
	{
	}

	FDSApiHandler::~FDSApiHandler()
	{
	}

	INT32 FDSApiHandler::ping(const INT32 input)
	{
		return 0;
	}

	INT32 FDSApiHandler::authenticateUser(
		const std::string& authToken,
		const INT32 sessionId,
		const std::string& username,
		const std::string& password,
		const std::string& domain)
	{
		INT32 authStatus = -1;

		PropertyManager* propertyManager = APP_CONTEXT.getPropertyManager();
		ConnectionStore* connectionStore = APP_CONTEXT.getConnectionStore();
		SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
		SessionPtr authSession = sessionStore->getSession(sessionId);

		// Check to see if the session is valid.
		if (!authSession || !authSession->isAuthSession())
		{
			WLog_Print(logger_FDSApiHandler, WLOG_ERROR,
				"Session id %d is not a valid session",
				sessionId);
			return -1;
		}

		// Check for an associated connection.
		UINT32 connectionId = connectionStore->getConnectionIdForSessionId(sessionId);
		ConnectionPtr connection = connectionStore->getConnection(connectionId);

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
				sessionId, username.c_str(), domain.c_str());
			return authStatus;
		}

		// The user has been authenticated.  Now we can either 1) connect
		// to a disconnected session or 2) create a new user session.

		SessionPtr userSession;
		bool isReconnectAllowed;
		bool isNewSession;

		if (!propertyManager->getPropertyBool("session.reconnect", isReconnectAllowed))
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

		if (!propertyManager->getPropertyString("module", moduleConfigName))
		{
			moduleConfigName = "X11";
		}
		userSession->setModuleConfigName(moduleConfigName);

		connection->setSessionId(userSession->getSessionId());

		connection->getClientInformation()->with = userSession->getClientDisplayWidth();
		connection->getClientInformation()->height = userSession->getClientDisplayHeight();
		connection->getClientInformation()->colordepth = userSession->getClientDisplayColorDepth();

		if (isNewSession)
		{
			WLog_Print(logger_FDSApiHandler, WLOG_INFO,
				"Starting new session %d for user '%s' in domain '%s'",
				sessionId, username.c_str(), domain.c_str());

			std::string pipeName;

			if (!userSession->startModule(pipeName))
			{
				WLog_Print(logger_FDSApiHandler, WLOG_ERROR,
					"ModuleConfig %s does not start properly for user '%s' in domain '%s'",
					moduleConfigName.c_str(), username.c_str(), domain.c_str());
				return -1;
			}
		}

		WLog_Print(logger_FDSApiHandler, WLOG_INFO,
			"Switching from session %d to session %d",
			sessionId, userSession->getSessionId());

		TaskSwitchToPtr switchToTask;
		switchToTask = TaskSwitchToPtr(new TaskSwitchTo());
		switchToTask->setConnectionId(connectionId);
		switchToTask->setServiceEndpoint(userSession->getPipeName());
		switchToTask->setOldSessionId(sessionId);
		switchToTask->setNewSessionId(userSession->getSessionId());
		APP_CONTEXT.addTask(switchToTask);

		// The user session is now active.
		userSession->setConnectState(WTSActive);

		return authStatus;
	}

	UINT32 FDSApiHandler::virtualChannelOpen(
		std::string& _return,
		const std::string& authToken,
		const INT32 sessionId,
		const std::string& virtualName)
	{
		UINT32 channelPort = 0;
		CallOutVirtualChannelOpen openCall;
		ConnectionStore* connectionStore = APP_CONTEXT.getConnectionStore();
		UINT32 connectionId = connectionStore->getConnectionIdForSessionId(sessionId);

		openCall.setConnectionId(connectionId);
		openCall.setChannelName(virtualName);

		APP_CONTEXT.getRpcOutgoingQueue()->addElement(&openCall);
		WaitForSingleObject(openCall.getAnswerHandle(), INFINITE);

		if (openCall.getResult() == 0)
		{
			_return = openCall.getChannelGuid();
			channelPort = openCall.getChannelPort();
		}
		else
		{
			_return = "";
		}

		return channelPort;
	}

	UINT32 FDSApiHandler::virtualChannelOpenEx(
		std::string& _return,
		const std::string& authToken,
		const INT32 sessionId,
		const std::string& virtualName,
		const INT32 flags)
	{
		return 0;
	}

	bool FDSApiHandler::virtualChannelClose(
		const std::string& authToken,
		const INT32 sessionId,
		const std::string& virtualName)
	{
		return false;
	}

	bool FDSApiHandler::disconnectSession(
		const std::string& authToken,
		const INT32 sessionId,
		const bool wait)
	{
		SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
		SessionPtr session = sessionStore->getSession(sessionId);

		if (session)
		{
			/* TODO: Disconnect the session. */
		}

		return false;
	}

	bool FDSApiHandler::logoffSession(
		const std::string& authToken,
		const INT32 sessionId,
		const bool wait)
	{
		SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
		SessionPtr session = sessionStore->getSession(sessionId);

		if (session)
		{
			/* TODO: Logoff the session. */
		}

		return false;
	}

	bool FDSApiHandler::shutdownSystem(
		const std::string& authToken,
		const INT32 shutdownFlag)
	{
		/* TODO: Shutdown the system. */

		return false;
	}

	void FDSApiHandler::enumerateSessions(
		TReturnEnumerateSessions& _return,
		const std::string& authToken,
		const INT32 Version)
	{
		DWORD count;
		DWORD index;

		SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
		std::list<SessionPtr> sessions = sessionStore->getAllSessions();

		count = sessions.size();
		TSessionInfoList list(count);

		std::list<SessionPtr>::iterator session = sessions.begin();

		for (index = 0; index < count; index++)
		{
			list.at(index).sessionId = (*session)->getSessionId();
			list.at(index).winStationName = (*session)->getWinStationName();
			list.at(index).connectState = (*session)->getConnectState();
			session++;
		}

		_return.sessionInfoList = list;

		_return.returnValue = true;
	}

	void FDSApiHandler::querySessionInformation(
		TReturnQuerySessionInformation& _return,
		const std::string& authToken,
		const INT32 sessionId,
		const INT32 infoClass)
	{
		SessionStore* sessionStore = APP_CONTEXT.getSessionStore();
		SessionPtr session = sessionStore->getSession(sessionId);

		_return.returnValue = false;

		if (session)
		{
			WTS_CONNECTSTATE_CLASS connectState = session->getConnectState();
			bool isClientConnected;

			if ((connectState == WTSConnected) || (connectState == WTSActive))
			{
				isClientConnected = true;
			}
			else
			{
				isClientConnected = false;
			}

			switch (infoClass)
			{
				case WTSSessionId:
					_return.infoValue.int32Value = session->getSessionId();
					_return.returnValue = true;
					break;

				case WTSUserName:
					_return.infoValue.stringValue = session->getUserName();
					_return.returnValue = true;
					break;

				case WTSWinStationName:
					_return.infoValue.stringValue = session->getWinStationName();
					_return.returnValue = true;
					break;

				case WTSDomainName:
					_return.infoValue.stringValue = session->getDomain();
					_return.returnValue = true;
					break;

				case WTSConnectState:
					_return.infoValue.int32Value = session->getConnectState();
					_return.returnValue = true;
					break;

				case WTSClientBuildNumber:
					if (isClientConnected)
					{
						_return.infoValue.int32Value = session->getClientBuildNumber();
						_return.returnValue = true;
					}
					break;

				case WTSClientName:
					if (isClientConnected)
					{
						_return.infoValue.stringValue = session->getClientName();
						_return.returnValue = true;
					}
					break;

				case WTSClientProductId:
					if (isClientConnected)
					{
						_return.infoValue.int16Value = session->getClientProductId();
						_return.returnValue = true;
					}
					break;

				case WTSClientHardwareId:
					if (isClientConnected)
					{
						_return.infoValue.int32Value = session->getClientHardwareId();
						_return.returnValue = true;
					}
					break;

				case WTSClientAddress:
					if (isClientConnected)
					{
						_return.infoValue.stringValue = session->getClientAddress();
						_return.returnValue = true;
					}
					break;

				case WTSClientDisplay:
					if (isClientConnected)
					{
						_return.infoValue.displayValue.displayWidth = session->getClientDisplayWidth();
						_return.infoValue.displayValue.displayHeight = session->getClientDisplayHeight();
						_return.infoValue.displayValue.colorDepth = session->getClientDisplayColorDepth();
						_return.returnValue = true;
					}
					break;

				case WTSClientProtocolType:
					if (isClientConnected)
					{
						_return.infoValue.int16Value = session->getClientProtocolType();
						_return.returnValue = true;
					}
					break;

				default:
					break;
			}
		}
	}
}


