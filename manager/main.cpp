/**
 * main of session manager
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include <fcntl.h>

#include <iostream>

#include <session/ApplicationContext.h>

#include <winpr/debug.h>
#include <winpr/synch.h>
#include <winpr/cmdline.h>
#include <winpr/environment.h>
#include <winpr/library.h>
#include <winpr/path.h>
#include <winpr/wlog.h>

#define FREERDS_PID_FILE	"freerds-manager.pid"

static HANDLE g_TermEvent = NULL;

void shutdown(int signal)
{
	fprintf(stderr, "shutdown due to signal %d\n", signal);

	if (g_TermEvent)
		SetEvent(g_TermEvent);
}

COMMAND_LINE_ARGUMENT_A freerds_session_manager_args[] =
{
	{ "kill", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, NULL, "kill daemon" },
	{ "no-daemon", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, "nodaemon", "no daemon" },
	{ NULL, 0, NULL, NULL, NULL, -1, NULL, NULL }
};

#ifndef _WIN32

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

int freerds_kill_daemon(const char* pid_file)
{
	int pid;
	char text[32];
	FILE* fd = NULL;

	if (PathFileExistsA(pid_file))
	{
		fd = fopen(pid_file, "r");
	}

	if (!fd)
		return -1;

	ZeroMemory(text, sizeof(text));
	fread((void*) text, sizeof(text) - 1, 1, fd);
	fclose(fd);

	pid = atoi(text);

	if (pid <= 0)
		return -1;

	fprintf(stderr, "stopping FreeRDS manager (pid %d)\n", pid);

	kill(pid, SIGTERM);

	return 1;
}

int freerds_daemonize_process(const char* pid_file)
{
	int pid;
	FILE* fd;
	char text[32];
	char logFilePath[512];
	char* logFileName;
	char* delimiter;
	wLog* wLog;
	wLogFileAppender* wLogAppender;

	if (!PathFileExistsA(FREERDS_VAR_PATH))
		CreateDirectoryA(FREERDS_VAR_PATH, NULL);

	if (!PathFileExistsA(FREERDS_PID_PATH))
		CreateDirectoryA(FREERDS_PID_PATH, NULL);

	/* make sure we can write to pid file */

	fd = fopen(pid_file, "w+");

	if (!fd)
		return -1;

	if (fwrite((void*) "0", 1, 1, fd) == -1)
		return -1;

	fclose(fd);
	DeleteFileA(pid_file);

	/* start of daemonizing code */

	pid = fork();

	if (pid < 0)
		return -1;

	if (0 != pid)
	{
		/* parent process */
		fprintf(stderr, "starting FreeRDS manager (pid %d)\n", pid);
		return 0;
	}

	/* write the pid to file */

	pid = GetCurrentProcessId();

	fd = fopen(pid_file, "w+");

	if (!fd)
	{
		fprintf(stderr, "problem opening pid file: %s\n", pid_file);
	}
	else
	{
		sprintf_s(text, sizeof(text) - 1, "%d", pid);
		fwrite((void*) text, strlen(text), 1, fd);
		fclose(fd);
	}

	close(0);
	close(1);
	close(2);

	open("/dev/null", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	open("/dev/null", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	open("/dev/null", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	/* Set up logging. */
	GetModuleFileName(NULL, logFilePath, sizeof(logFilePath));
	strcat(logFilePath, ".log");

	delimiter = strrchr(logFilePath, '/');
	logFileName = delimiter + 1;
	*delimiter = '\0';

	SetEnvironmentVariable("WLOG_APPENDER", "FILE");
	SetEnvironmentVariable("WLOG_FILEAPPENDER_OUTPUT_FILE_PATH", logFilePath);
	SetEnvironmentVariable("WLOG_FILEAPPENDER_OUTPUT_FILE_NAME", logFileName);

	wLog = WLog_GetRoot();
	WLog_SetLogAppenderType(wLog, WLOG_APPENDER_FILE);
	wLogAppender = (wLogFileAppender*) WLog_GetLogAppender(wLog);
	WLog_FileAppender_SetOutputFilePath(wLog, wLogAppender, logFilePath);
	WLog_FileAppender_SetOutputFileName(wLog, wLogAppender, logFileName);

	WLog_DBG("main", "started");

	/* end of daemonizing code */

	return 1;
}

#else

int freerds_kill_daemon(const char* pid_file) { return -1; }
int freerds_daemonize_process(const char* pid_file) { return -1; }

#endif

int main(int argc, char** argv)
{
	int status;
	DWORD flags;
	BOOL daemon;
	BOOL kill_process;
	char pid_file[256];
	COMMAND_LINE_ARGUMENT_A* arg;

	daemon = TRUE;
	kill_process = FALSE;

	flags = 0;
	flags |= COMMAND_LINE_SIGIL_DASH;
	flags |= COMMAND_LINE_SIGIL_DOUBLE_DASH;
	flags |= COMMAND_LINE_SIGIL_ENABLE_DISABLE;
	flags |= COMMAND_LINE_SEPARATOR_COLON;

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
			kill_process = TRUE;
		}
		CommandLineSwitchCase(arg, "no-daemon")
		{
			daemon = FALSE;
		}
		CommandLineSwitchEnd(arg)
	}
	while ((arg = CommandLineFindNextArgumentA(arg)) != NULL);

	sprintf_s(pid_file, sizeof(pid_file), "%s/%s", FREERDS_PID_PATH, FREERDS_PID_FILE);

	if (kill_process)
	{
		status = freerds_kill_daemon(pid_file);
		return 0;
	}

	if (PathFileExistsA(pid_file))
	{
		fprintf(stderr, "The FreeRDS manager appears to be running\n");
		fprintf(stderr, "If this is not the case, delete %s and try again\n", pid_file);
		return 0;
	}

	if (daemon)
	{
		status = freerds_daemonize_process(pid_file);

		if (!status)
			return 0; /* parent process */

		if (status < 0)
			return 1;
	}

	freerds_crash_setup();

	g_TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

#ifndef _WIN32
	signal(SIGINT, shutdown);
	signal(SIGKILL, shutdown);
	signal(SIGTERM, shutdown);

	/*
	 * Ignore SIGPIPE - This can occur normally due to the use of
	 * named pipes as a means of interprocess communication.
	 * It shouldn't result in terminating the entire process.
	 */
	signal(SIGPIPE, SIG_IGN);
#endif

	APP_CONTEXT.startRPCEngine();
	APP_CONTEXT.loadModulesFromPath(APP_CONTEXT.getLibraryPath());

	APP_CONTEXT.getPropertyManager()->loadProperties(APP_CONTEXT.getSystemConfigPath() + "/config.ini");
	APP_CONTEXT.getPropertyManager()->saveProperties(APP_CONTEXT.getSystemConfigPath() + "/config.ini");

	APP_CONTEXT.startTaskExecutor();
	APP_CONTEXT.startSessionTimoutMonitor();

	WaitForSingleObject(g_TermEvent, INFINITE);
	CloseHandle(g_TermEvent);
	g_TermEvent = NULL;

	APP_CONTEXT.stopTaskExecutor();
	APP_CONTEXT.stopRPCEngine();

	DeleteFileA(pid_file);

	return 0;
}
