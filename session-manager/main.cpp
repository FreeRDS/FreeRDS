/**
 * main of session manager
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

#include "config.h"
#include <iostream>

#include <appcontext/ApplicationContext.h>

#ifndef _WIN32
#include <winpr/wtypes.h>
#include <winpr/synch.h>
#include <signal.h>
#endif

using namespace std;

static HANDLE TermEvent;

#ifndef _WIN32
void shutdown(int signal)
{
	SetEvent(TermEvent);
}
#endif


int main(void)
{
#ifdef _WIN32
	std::string test;
#endif
	APP_CONTEXT.startRPCEngine();
	APP_CONTEXT.loadModulesFromPath(APP_CONTEXT.getLibraryPath());

	cout << "Hello session manager" << endl;
#ifndef _WIN32
	TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	signal(SIGINT, shutdown);
	signal(SIGKILL, shutdown);
	signal(SIGPIPE, shutdown);

	WaitForSingleObject(TermEvent, INFINITE);
	CloseHandle(TermEvent);
#else // _WIN32
	cin >>test;
#endif // _WIN32

	APP_CONTEXT.stopRPCEngine();
	return 0;
}
