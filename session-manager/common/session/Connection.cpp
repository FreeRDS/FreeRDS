/**
 * Connection class
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

#include <winpr/crt.h>
#include <winpr/wlog.h>
#include <winpr/sspicli.h>
#include <winpr/environment.h>

#include <appcontext/ApplicationContext.h>

#include "Connection.h"

namespace freerds
{
	namespace sessionmanager
	{
		namespace session
		{
			static DWORD gConnectionId = 1;

			static wLog* logger_Connection = WLog_Get("freerds.sessionmanager.session.connection");

			Connection::Connection(DWORD connectionId)
				: mConnectionId(connectionId)
			{
				mPipeName = (char*) malloc(256);
				sprintf_s(mPipeName, 256, "\\\\.\\pipe\\FreeRDS_Connection_%d", (int) mConnectionId);
			}

			Connection::~Connection()
			{

			}

			char* Connection::getPipeName()
			{
				return mPipeName;
			}

			Connection* Connection::create()
			{
				Connection* connection = new Connection(gConnectionId++);
				return connection;
			}
		}
	}
}

