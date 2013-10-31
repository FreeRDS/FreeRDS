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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xrdp.h"
#include <freerds/icp.h>

#include "os_calls.h"

#include <winpr/cmdline.h>
#include <freerds/icp_client_stubs.h>

char* RdsModuleName = NULL;
static HANDLE g_TermEvent = NULL;
static xrdpListener* g_listen = NULL;

COMMAND_LINE_ARGUMENT_A xrdp_ng_args[] =
{
	{ "kill", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, NULL, "kill daemon" },
	{ "nodaemon", COMMAND_LINE_VALUE_FLAG, "", NULL, NULL, -1, NULL, "no daemon" },
	{ "module", COMMAND_LINE_VALUE_REQUIRED, "<module name>", NULL, NULL, -1, NULL, "module name" },
	{ NULL, 0, NULL, NULL, NULL, -1, NULL, NULL }
};

void xrdp_shutdown(int sig)
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

void pipe_sig(int sig_num)
{
	/* do nothing */
	printf("got XRDP SIGPIPE(%d)\n", sig_num);
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
	COMMAND_LINE_ARGUMENT_A* arg;

	g_init("xrdp");

	no_daemon = kill = 0;

	g_snprintf(cfg_file, 255, "%s/xrdp.ini", RDS_CFG_PATH);

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
		CommandLineSwitchCase(arg, "module")
		{
			RdsModuleName = _strdup(arg->Value);
		}

		CommandLineSwitchEnd(arg)
	}
	while ((arg = CommandLineFindNextArgumentA(arg)) != NULL);

	g_snprintf(pid_file, 255, "%s/xrdp-ng.pid", RDS_PID_PATH);

	if (kill)
	{
		printf("stopping xrdp\n");
		/* read the xrdp.pid file */
		fd = -1;

		if (g_file_exist(pid_file)) /* xrdp-ng.pid */
		{
			fd = g_file_open(pid_file); /* xrdp-ng.pid */
		}

		if (fd == -1)
		{
			printf("problem opening to xrdp-ng.pid [%s]\n", pid_file);
			printf("maybe its not running\n");
		}
		else
		{
			memset(text, 0, 32);
			g_file_read(fd, (unsigned char*) text, 31);
			pid = atoi(text);
			printf("stopping process id %d\n", pid);

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
		printf("It looks like xrdp is already running,\n");
		printf("if not delete the xrdp-ng.pid file and try again\n");
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
			printf("running in daemon mode with no access to pid files, quitting\n");
			g_deinit();
			g_exit(0);
		}

		if (g_file_write(fd, (unsigned char*) "0", 1) == -1)
		{
			printf("running in daemon mode with no access to pid files, quitting\n");
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
			printf("problem forking\n");
			g_deinit();
			g_exit(1);
		}

		if (0 != pid)
		{
			printf("process %d started ok\n", pid);
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
			printf("trying to write process id to xrdp-ng.pid\n");
			printf("problem opening xrdp-ng.pid\n");
		}
		else
		{
			g_sprintf(text, "%d", pid);
			g_file_write(fd, (unsigned char*) text, strlen(text));
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
	printf("starting icp and waiting for session manager \n");
	freerds_icp_start();
	printf("connected to session manager\n");

	xrdp_listen_main_loop(g_listen);
	xrdp_listen_delete(g_listen);

	CloseHandle(g_TermEvent);

	/* only main process should delete pid file */
	if ((!no_daemon) && (pid == g_getpid()))
	{
		/* delete the xrdp-ng.pid file */
		g_file_delete(pid_file);
	}

	freerds_icp_shutdown();
	g_deinit();

	return 0;
}
