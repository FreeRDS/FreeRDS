/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <signal.h>
#endif

#include "freerds.h"

#include <winpr/crt.h>
#include <winpr/path.h>
#include <winpr/debug.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/cmdline.h>
#include <winpr/environment.h>
#include <winpr/library.h>
#include <winpr/wtsapi.h>
#include <winpr/wlog.h>

#include "rpc.h"

extern rdsServer* g_Server;

static HANDLE g_TermEvent = NULL;
static char* freerds_home_path = NULL;

char* freerds_get_home_path()
{
	if (!freerds_home_path)
	{
		char* p;
		int length;
		char separator;
		char moduleFileName[4096];

		separator = PathGetSeparatorA(PATH_STYLE_NATIVE);
		GetModuleFileNameA(NULL, moduleFileName, sizeof(moduleFileName));

		p = strrchr(moduleFileName, separator);
		*p = '\0';
		p = strrchr(moduleFileName, separator);
		*p = '\0';

		length = strlen(moduleFileName);
		freerds_home_path = (char*) malloc(length + 1);
		CopyMemory(freerds_home_path, moduleFileName, length);
		freerds_home_path[length] = '\0';
	}

	return freerds_home_path;
}

COMMAND_LINE_ARGUMENT_A freerds_server_args[] =
{
	{ "kill", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, NULL, "kill daemon" },
	{ "no-daemon", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, "nodaemon", "no daemon" },
	{ NULL, 0, NULL, NULL, NULL, -1, NULL, NULL }
};

void freerds_shutdown(int sig)
{
	DWORD threadId;

	threadId = GetCurrentThreadId();

	printf("shutting down\n");
	printf("signal %d threadId %d\n", sig, (int) threadId);

	if (WaitForSingleObject(g_TermEvent, 0) != WAIT_OBJECT_0)
		SetEvent(g_TermEvent);
}

int g_is_term(void)
{
	return (WaitForSingleObject(g_TermEvent, 0) == WAIT_OBJECT_0) ? 1 : 0;
}

void g_set_term(int in_val)
{
	if (in_val)
		SetEvent(g_TermEvent);
	else
		ResetEvent(g_TermEvent);
}

HANDLE g_get_term_event(void)
{
	return g_TermEvent;
}

void freerds_crash_handler(int signum)
{
	void* bt;
	int index;
	char** symbols;
	size_t count = 0;

	WLog_FATAL("CRASH", "fatal signal %d received", signum);

	bt = winpr_backtrace(32);

	symbols = winpr_backtrace_symbols(bt, &count);

	for (index = 0; index < count; index++)
	{
		WLog_FATAL("CRASH", "%s", symbols[index]);
	}

	winpr_backtrace_free(bt);

	exit(-1);
}

void freerds_crash_setup()
{
	int index;
	sigset_t set;
	int signals[] = { SIGSEGV, SIGILL, SIGBUS, SIGFPE, SIGSTKFLT, SIGSYS, SIGPIPE };
	int count = sizeof(signals) / sizeof(int);

	sigemptyset(&set);

	for (index = 0; index < count; index++)
	{
		sigaddset(&set, signals[index]);
	}

	sigprocmask(SIG_UNBLOCK, &set, NULL);

	for (index = 0; index < count; index++)
	{
		signal(signals[index], freerds_crash_handler);
	}
}

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
	rdsServer* server;
	COMMAND_LINE_ARGUMENT_A* arg;
#ifndef _WIN32
	sigset_t set;
#endif

	WTSRegisterWtsApiFunctionTable(FreeRDP_InitWtsApi());

	no_daemon = kill_process = 0;

	flags = COMMAND_LINE_SEPARATOR_SPACE;
	flags |= COMMAND_LINE_SIGIL_DASH | COMMAND_LINE_SIGIL_DOUBLE_DASH;

	status = CommandLineParseArgumentsA(argc, (const char**) argv,
			freerds_server_args, flags, NULL, NULL, NULL);

	arg = freerds_server_args;

	do
	{
		if (!(arg->Flags & COMMAND_LINE_VALUE_PRESENT))
			continue;

		CommandLineSwitchStart(arg)

		CommandLineSwitchCase(arg, "kill")
		{
			kill_process = 1;
		}
		CommandLineSwitchCase(arg, "no-daemon")
		{
			no_daemon = 1;
		}

		CommandLineSwitchEnd(arg)
	}
	while ((arg = CommandLineFindNextArgumentA(arg)) != NULL);

	sprintf_s(pid_file, 255, "%s/freerds-server.pid", FREERDS_PID_PATH);

#ifndef _WIN32
	if (kill_process)
	{
		printf("stopping FreeRDS server\n");

		fd = NULL;

		if (PathFileExistsA(pid_file))
		{
			fd = fopen(pid_file, "r");
		}

		if (!fd)
		{
			printf("problem opening pid file [%s]\n", pid_file);
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
#endif

	if (PathFileExistsA(pid_file))
	{
		printf("It looks like FreeRDS server is already running,\n");
		printf("if not delete the freerds-server.pid file and try again\n");
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

#ifndef WIN32
	/* block all signals per default */
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);

	if (!no_daemon)
	{
		char logFilePath[512];
		char* logFileName;
		char* delimiter;

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
			printf("trying to write process id to freerds-server.pid\n");
			printf("problem opening freerds-server.pid\n");
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
		
		GetModuleFileName(NULL, logFilePath, sizeof(logFilePath));
		strcat(logFilePath, ".log");

		delimiter = strrchr(logFilePath, '/');
		logFileName = delimiter + 1;
		*delimiter = '\0';

		SetEnvironmentVariable("WLOG_APPENDER", "FILE");
		SetEnvironmentVariable("WLOG_FILEAPPENDER_OUTPUT_FILE_PATH", logFilePath);
		SetEnvironmentVariable("WLOG_FILEAPPENDER_OUTPUT_FILE_NAME", logFileName);

		WLog_Init();

		/* end of daemonizing code */
	}

	/* unblock required signals */
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGPIPE);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGSEGV);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
#endif

	server = freerds_server_new();
	g_Server = server;

#ifndef _WIN32
	signal(SIGINT, freerds_shutdown);
	signal(SIGTERM, freerds_shutdown);
	freerds_crash_setup();
#endif

	pid = GetCurrentProcessId();

	freerds_get_home_path();
	g_TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	freerds_server_main_loop(server);

	freerds_server_free(server);
	g_Server = NULL;

	CloseHandle(g_TermEvent);

	/* only main process should delete pid file */
	if ((!no_daemon) && (pid == GetCurrentProcessId()))
	{
		DeleteFileA(pid_file);
	}

	return 0;
}
