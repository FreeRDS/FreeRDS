/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
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
 *
 * main program
 */

#include "xrdp.h"
#include "log.h"

#include <winpr/cmdline.h>

static HANDLE g_TermEvent = NULL;
static xrdpListener* g_listen = NULL;

COMMAND_LINE_ARGUMENT_A xrdp_ng_args[] =
{
	{ "kill", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, NULL, "kill daemon" },
	{ "nodaemon", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, NULL, "no daemon" },
	{ NULL, 0, NULL, NULL, NULL, -1, NULL, NULL }
};

void xrdp_shutdown(int sig)
{
	DWORD threadId;

	threadId = GetCurrentThreadId();
	g_writeln("shutting down");
	g_writeln("signal %d threadId %p", sig, threadId);

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

void pipe_sig(int sig_num)
{
	/* do nothing */
	g_writeln("got XRDP SIGPIPE(%d)", sig_num);
}

int main(int argc, char** argv)
{
	int fd;
	int pid;
	int kill;
	int status;
	DWORD flags;
	int no_daemon;
	char text[256];
	char pid_file[256];
	char cfg_file[256];
	enum logReturns error;
	COMMAND_LINE_ARGUMENT_A* arg;

	g_init("xrdp");

	no_daemon = kill = 0;

	g_snprintf(cfg_file, 255, "%s/xrdp.ini", XRDP_CFG_PATH);

	/* starting logging subsystem */
	error = log_start(cfg_file, "XRDP");

	if (error != LOG_STARTUP_OK)
	{
		switch (error)
		{
			case LOG_ERROR_MALLOC:
				g_writeln("error on malloc. cannot start logging. quitting.");
				break;

			case LOG_ERROR_FILE_OPEN:
				g_writeln("error opening log file [%s]. quitting.", getLogFile(text, 255));
				break;

			default:
				g_writeln("log_start error");
				break;
		}

		g_deinit();
		g_exit(1);
	}

	flags = COMMAND_LINE_SEPARATOR_SPACE;
	flags |= COMMAND_LINE_SIGIL_DASH | COMMAND_LINE_SIGIL_DOUBLE_DASH;

	status = CommandLineParseArgumentsA(argc, (const char**) argv,
			xrdp_ng_args, flags, NULL, NULL, NULL);

	arg = xrdp_ng_args;

	do
	{
		if (!(arg->Flags & COMMAND_LINE_VALUE_PRESENT))
			continue;

		CommandLineSwitchStart(arg)

		CommandLineSwitchCase(arg, "kill")
		{
			kill = 1;
		}
		CommandLineSwitchCase(arg, "nodaemon")
		{
			no_daemon = 1;
		}

		CommandLineSwitchEnd(arg)
	}
	while ((arg = CommandLineFindNextArgumentA(arg)) != NULL);

	g_snprintf(pid_file, 255, "%s/xrdp-ng.pid", XRDP_PID_PATH);

	if (kill)
	{
		g_writeln("stopping xrdp");
		/* read the xrdp.pid file */
		fd = -1;

		if (g_file_exist(pid_file)) /* xrdp-ng.pid */
		{
			fd = g_file_open(pid_file); /* xrdp-ng.pid */
		}

		if (fd == -1)
		{
			g_writeln("problem opening to xrdp-ng.pid [%s]", pid_file);
			g_writeln("maybe its not running");
		}
		else
		{
			g_memset(text, 0, 32);
			g_file_read(fd, (unsigned char*) text, 31);
			pid = g_atoi(text);
			g_writeln("stopping process id %d", pid);

			if (pid > 0)
			{
				g_sigterm(pid);
			}

			g_file_close(fd);
		}

		g_deinit();
		g_exit(0);
	}

	if (g_file_exist(pid_file)) /* xrdp-ng.pid */
	{
		g_writeln("It looks like xrdp is already running,");
		g_writeln("if not delete the xrdp-ng.pid file and try again");
		g_deinit();
		g_exit(0);
	}

	if (!no_daemon)
	{

		/* make sure containing directory exists */
		g_create_path(pid_file);

		/* make sure we can write to pid file */
		fd = g_file_open(pid_file); /* xrdp-ng.pid */

		if (fd == -1)
		{
			g_writeln("running in daemon mode with no access to pid files, quitting");
			g_deinit();
			g_exit(0);
		}

		if (g_file_write(fd, (unsigned char*) "0", 1) == -1)
		{
			g_writeln("running in daemon mode with no access to pid files, quitting");
			g_deinit();
			g_exit(0);
		}

		g_file_close(fd);
		g_file_delete(pid_file);
	}

	if (!no_daemon)
	{
		/* start of daemonizing code */
		pid = g_fork();

		if (pid == -1)
		{
			g_writeln("problem forking");
			g_deinit();
			g_exit(1);
		}

		if (0 != pid)
		{
			g_writeln("process %d started ok", pid);
			/* exit, this is the main process */
			g_deinit();
			g_exit(0);
		}

		g_sleep(1000);
		/* write the pid to file */
		pid = g_getpid();
		fd = g_file_open(pid_file); /* xrdp-ng.pid */

		if (fd == -1)
		{
			g_writeln("trying to write process id to xrdp-ng.pid");
			g_writeln("problem opening xrdp-ng.pid");
			g_writeln("maybe no rights");
		}
		else
		{
			g_sprintf(text, "%d", pid);
			g_file_write(fd, (unsigned char*) text, g_strlen(text));
			g_file_close(fd);
		}

		g_sleep(1000);
		g_file_close(0);
		g_file_close(1);
		g_file_close(2);
		g_file_open("/dev/null");
		g_file_open("/dev/null");
		g_file_open("/dev/null");
		/* end of daemonizing code */
	}

	g_listen = xrdp_listen_create();
	g_signal_user_interrupt(xrdp_shutdown); /* SIGINT */
	g_signal_kill(xrdp_shutdown); /* SIGKILL */
	g_signal_pipe(pipe_sig); /* SIGPIPE */
	g_signal_terminate(xrdp_shutdown); /* SIGTERM */
	pid = g_getpid();

	g_TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	xrdp_listen_main_loop(g_listen);
	xrdp_listen_delete(g_listen);

	CloseHandle(g_TermEvent);

	/* only main process should delete pid file */
	if ((!no_daemon) && (pid == g_getpid()))
	{
		/* delete the xrdp-ng.pid file */
		g_file_delete(pid_file);
	}

	g_deinit();

	return 0;
}
