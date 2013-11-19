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

#include <winpr/wtypes.h>
#include <winpr/synch.h>
#include <signal.h>
#include <fcntl.h>
#include <winpr/cmdline.h>
#include <winpr/path.h>

using namespace std;

static HANDLE gTermEvent;

void shutdown(int signal)
{
	SetEvent(gTermEvent);
}

COMMAND_LINE_ARGUMENT_A freerds_session_manager_args[] =
{
	{ "kill", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, NULL, "kill daemon" },
	{ "nodaemon", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, NULL, "no daemon" },
	{ NULL, 0, NULL, NULL, NULL, -1, NULL, NULL }
};



int main(int argc, char** argv)
{
	int pid;
	FILE* fd;
	int status;
	DWORD flags;
	int no_daemon;
	int kill_process;
	char text[256];
	char pid_file[256];
	COMMAND_LINE_ARGUMENT_A* arg;

	no_daemon = kill_process = 0;

	flags = COMMAND_LINE_SEPARATOR_SPACE;
	flags |= COMMAND_LINE_SIGIL_DASH | COMMAND_LINE_SIGIL_DOUBLE_DASH;

	status = CommandLineParseArgumentsA(argc, (const char**) argv,
			freerds_session_manager_args, flags, NULL, NULL, NULL);

	arg = freerds_session_manager_args;

	do
	{
		if (!(arg->Flags & COMMAND_LINE_VALUE_PRESENT))
			continue;

		CommandLineSwitchStart(arg)

		CommandLineSwitchCase(arg, "kill")
		{
			kill_process = 1;
		}
		CommandLineSwitchCase(arg, "nodaemon")
		{
			no_daemon = 1;
		}
		CommandLineSwitchEnd(arg)
	}
	while ((arg = CommandLineFindNextArgumentA(arg)) != NULL);

	sprintf_s(pid_file, 255, "%s/freerds-sessionmanager.pid", FREERDS_PID_PATH);

	if (kill_process)
	{
		printf("stopping FreeRDS\n");

		fd = NULL;

		if (PathFileExistsA(pid_file))
		{
			fd = fopen(pid_file, "r");
		}

		if (!fd)
		{
			printf("problem opening freerds-sessionmanager.pid [%s]\n", pid_file);
			printf("maybe its not running\n");
		}
		else
		{
			ZeroMemory(text, 32);
			fread((void*) text, 31, 1, fd);
			pid = atoi(text);
			printf("stopping process id %d\n", pid);

			if (pid > 0)
			{
				kill(pid, SIGTERM);
			}

			fclose(fd);
		}

		return 0;
	}

	if (PathFileExistsA(pid_file))
	{
		printf("It looks like FreeRDS Session Manager is already running,\n");
		printf("if not delete the freerds-sessionmanager.pid file and try again\n");
		return 0;
	}

	if (!no_daemon)
	{
		if (!PathFileExistsA(FREERDS_VAR_PATH))
			CreateDirectoryA(FREERDS_VAR_PATH, NULL);

		if (!PathFileExistsA(FREERDS_PID_PATH))
			CreateDirectoryA(FREERDS_PID_PATH, NULL);

		/* make sure we can write to pid file */
		fd = fopen(pid_file, "w+");

		if (!fd)
		{
			printf("running in daemon mode with no access to pid files, quitting\n");
			return 0;
		}

		if (fwrite((void*) "0", 1, 1, fd) == -1)
		{
			printf("running in daemon mode with no access to pid files, quitting\n");
			return 0;
		}

		fclose(fd);
		DeleteFileA(pid_file);
	}

	if (!no_daemon)
	{
		/* start of daemonizing code */
		pid = fork();

		if (pid == -1)
		{
			printf("problem forking\n");
			return 1;
		}

		if (0 != pid)
		{
			printf("process %d started\n", pid);
			return 0;
		}

		Sleep(1000);

		/* write the pid to file */
		pid = GetCurrentProcessId();
		fd = fopen(pid_file, "w+");

		if (!fd)
		{
			printf("trying to write process id to freerds.pid\n");
			printf("problem opening freerds.pid\n");
		}
		else
		{
			sprintf_s(text, sizeof(text), "%d", pid);
			fwrite((void*) text, strlen(text), 1, fd);
			fclose(fd);
		}

		Sleep(1000);
		close(0);
		close(1);
		close(2);
		open("/dev/null", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		open("/dev/null", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		open("/dev/null", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		/* end of daemonizing code */
	}


	signal(SIGINT, shutdown);
	signal(SIGKILL, shutdown);
	signal(SIGPIPE, shutdown);
	signal(SIGTERM, shutdown);

	pid = GetCurrentProcessId();

	APP_CONTEXT.startRPCEngine();
	APP_CONTEXT.loadModulesFromPath(APP_CONTEXT.getLibraryPath());


	//APP_CONTEXT.getPropertyManager()->saveProperties(APP_CONTEXT.getSystemConfigPath() + "/config.ini");
	APP_CONTEXT.getPropertyManager()->loadProperties(APP_CONTEXT.getSystemConfigPath() + "/config.ini");


	cout << "Hello session manager" << endl;

	gTermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	WaitForSingleObject(gTermEvent, INFINITE);
	CloseHandle(gTermEvent);

	APP_CONTEXT.stopRPCEngine();

	/* only main process should delete pid file */
	if ((!no_daemon) && (pid == GetCurrentProcessId()))
	{
		DeleteFileA(pid_file);
	}


	return 0;
}
