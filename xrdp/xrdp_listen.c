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
 * listen for incoming connection
 */

#include "xrdp.h"
#include "log.h"

#define NEW_XRDP_LISTENER	1

#ifndef NEW_XRDP_LISTENER

struct xrdp_listener
{
	int status;
	struct trans* listen_trans; /* in tcp listen mode */
	xrdpList* process_list;
	tbus pro_done_event;
	struct xrdp_startup_params* startup_params;
};

/* 'g_process' is protected by the semaphore 'g_process_sem'.  One thread sets
   g_process and waits for the other to process it */
static tbus g_process_sem = 0;
static xrdpProcess *g_process = 0;

/*****************************************************************************/
static int xrdp_listen_create_pro_done(xrdpListener *self)
{
	int pid;
	char text[256];

	pid = g_getpid();
	g_snprintf(text, 255, "xrdp_%8.8x_listen_pro_done_event", pid);
	self->pro_done_event = g_create_wait_obj(text);

	if (self->pro_done_event == 0)
	{
		log_message(LOG_LEVEL_ERROR, "Failure creating pro_done_event");
	}

	return 0;
}

/*****************************************************************************/
xrdpListener* xrdp_listen_create(void)
{
	xrdpListener *self;

	self = (xrdpListener *) g_malloc(sizeof(xrdpListener), 1);
	xrdp_listen_create_pro_done(self);
	self->process_list = list_create();

	if (g_process_sem == 0)
	{
		g_process_sem = tc_sem_create(0);
	}

	self->listen_trans = trans_create(TRANS_MODE_TCP, 16, 16);

	if (self->listen_trans == 0)
	{
		log_message(LOG_LEVEL_ERROR, "xrdp_listen_create: trans_create failed");
	}

	return self;
}

/*****************************************************************************/
void xrdp_listen_delete(xrdpListener *self)
{
	if (self->listen_trans != 0)
	{
		trans_delete(self->listen_trans);
	}

	if (g_process_sem != 0)
	{
		tc_sem_delete(g_process_sem);
		g_process_sem = 0;
	}

	g_delete_wait_obj(self->pro_done_event);
	list_delete(self->process_list);
	g_free(self);
}

int xrdp_listen_set_startup_params(xrdpListener *self, struct xrdp_startup_params* startup_params)
{
	self->startup_params = startup_params;
	return 0;
}

/*****************************************************************************/
/* returns error */
static int xrdp_listen_add_pro(xrdpListener *self, xrdpProcess *process)
{
	list_add_item(self->process_list, (tbus) process);
	return 0;
}

/*****************************************************************************/
static int xrdp_listen_delete_done_pro(xrdpListener *self)
{
	int i;
	xrdpProcess* pro;

	for (i = self->process_list->count - 1; i >= 0; i--)
	{
		pro = (xrdpProcess*) list_get_item(self->process_list, i);

		if (pro != 0)
		{
			if (xrdp_process_get_status(pro) < 0)
			{
				xrdp_process_delete(pro);
				list_remove_item(self->process_list, i);
			}
		}
	}

	return 0;
}

/*****************************************************************************/
/* i can't get stupid in_val to work, hum using global var for now */
THREAD_RV THREAD_CC xrdp_process_run(void *in_val)
{
	xrdpProcess *process;

	DEBUG(("process started"));
	process = g_process;
	g_process = 0;
	tc_sem_inc(g_process_sem);
	xrdp_process_main_loop(process);
	DEBUG(("process done"));
	return 0;
}

/*****************************************************************************/
static int xrdp_listen_get_port_address(char *port, int port_bytes, char *address, int address_bytes, int *tcp_nodelay,
		int *tcp_keepalive, struct xrdp_startup_params *startup_param)
{
	int fd;
	int error;
	int index;
	char *val;
	xrdpList *names;
	xrdpList *values;
	char cfg_file[256];

	/* default to port 3389 */
	g_strncpy(port, "3389", port_bytes - 1);
	/* Default to all */
	g_strncpy(address, "0.0.0.0", address_bytes - 1);
	/* see if port or address is in xrdp.ini file */
	g_snprintf(cfg_file, 255, "%s/xrdp.ini", XRDP_CFG_PATH);
	fd = g_file_open(cfg_file);
	*tcp_nodelay = 0;
	*tcp_keepalive = 0;

	if (fd > 0)
	{
		names = list_create();
		names->auto_free = 1;
		values = list_create();
		values->auto_free = 1;

		if (file_read_section(fd, "globals", names, values) == 0)
		{
			for (index = 0; index < names->count; index++)
			{
				val = (char *) list_get_item(names, index);

				if (val != 0)
				{
					if (g_strcasecmp(val, "port") == 0)
					{
						val = (char *) list_get_item(values, index);
						error = g_atoi(val);

						if ((error > 0) && (error < 65000))
						{
							g_strncpy(port, val, port_bytes - 1);
						}
					}

					if (g_strcasecmp(val, "address") == 0)
					{
						val = (char *) list_get_item(values, index);
						g_strncpy(address, val, address_bytes - 1);
					}

					if (g_strcasecmp(val, "fork") == 0)
					{
						val = (char *) list_get_item(values, index);
						startup_param->fork = text2bool(val);
					}

					if (g_strcasecmp(val, "tcp_nodelay") == 0)
					{
						val = (char *) list_get_item(values, index);
						*tcp_nodelay = text2bool(val);
					}

					if (g_strcasecmp(val, "tcp_keepalive") == 0)
					{
						val = (char *) list_get_item(values, index);
						*tcp_keepalive = text2bool(val);
					}
				}
			}
		}

		list_delete(names);
		list_delete(values);
		g_file_close(fd);
	}

	/* startup_param overrides */
	if (startup_param->port[0] != 0)
	{
		g_strncpy(port, startup_param->port, port_bytes - 1);
	}

	return 0;
}

/*****************************************************************************/
static int xrdp_listen_fork(xrdpListener *self, struct trans *server_trans)
{
	int pid;
	xrdpProcess *process;

	pid = g_fork();

	if (pid == 0)
	{
		/* child */
		/* recreate some main globals */
		xrdp_child_fork();
		/* recreate the process done wait object, not used in fork mode */
		/* close, don't delete this */
		g_close_wait_obj(self->pro_done_event);
		xrdp_listen_create_pro_done(self);
		/* delete listener, child need not listen */
		trans_delete(self->listen_trans);
		self->listen_trans = 0;
		/* new connect instance */
		process = xrdp_process_create(self, 0);
		xrdp_process_set_transport(process, server_trans);
		g_process = process;
		xrdp_process_run(0);
		xrdp_process_delete(process);
		/* mark this process to exit */
		g_set_term(1);
		return 0;
	}

	/* parent */
	trans_delete(server_trans);
	return 0;
}

/*****************************************************************************/
/* a new connection is coming in */
int xrdp_listen_conn_in(struct trans *self, struct trans *new_self)
{
	xrdpProcess *process;
	xrdpListener *lis;

	lis = (xrdpListener *) (self->callback_data);

	if (lis->startup_params->fork)
	{
		return xrdp_listen_fork(lis, new_self);
	}

	process = xrdp_process_create(lis, lis->pro_done_event);

	if (xrdp_listen_add_pro(lis, process) == 0)
	{
		/* start thread */
		xrdp_process_set_transport(process, new_self);
		g_process = process;
		tc_thread_create(xrdp_process_run, 0);
		tc_sem_dec(g_process_sem); /* this will wait */
	}
	else
	{
		xrdp_process_delete(process);
	}

	return 0;
}

/*****************************************************************************/
/* wait for incoming connections */
int xrdp_listen_main_loop(xrdpListener *self)
{
	int error;
	int robjs_count;
	int cont;
	int timeout = 0;
	char port[128];
	char address[256];
	tbus robjs[8];
	tbus term_obj;
	tbus sync_obj;
	tbus done_obj;
	int tcp_nodelay;
	int tcp_keepalive;

	self->status = 1;

	if (xrdp_listen_get_port_address(port, sizeof(port), address, sizeof(address), &tcp_nodelay, &tcp_keepalive,
			self->startup_params) != 0)
	{
		log_message(LOG_LEVEL_ERROR, "xrdp_listen_main_loop: xrdp_listen_get_port failed");
		self->status = -1;
		return 1;
	}

	error = trans_listen_address(self->listen_trans, port, address);

	if (error == 0)
	{
		if (tcp_nodelay)
		{
			if (g_tcp_set_no_delay(self->listen_trans->sck))
			{
				log_message(LOG_LEVEL_ERROR, "Error setting tcp_nodelay");
			}
		}

		if (tcp_keepalive)
		{
			if (g_tcp_set_keepalive(self->listen_trans->sck))
			{
				log_message(LOG_LEVEL_ERROR, "Error setting tcp_keepalive");
			}
		}

		self->listen_trans->trans_conn_in = xrdp_listen_conn_in;
		self->listen_trans->callback_data = self;
		term_obj = g_get_term_event(); /*Global termination event */
		sync_obj = g_get_sync_event();
		done_obj = self->pro_done_event;
		cont = 1;

		while (cont)
		{
			/* build the wait obj list */
			robjs_count = 0;
			robjs[robjs_count++] = term_obj;
			robjs[robjs_count++] = sync_obj;
			robjs[robjs_count++] = done_obj;
			timeout = -1;

			if (self->listen_trans != 0)
			{
				if (trans_get_wait_objs(self->listen_trans, robjs, &robjs_count) != 0)
				{
					log_message(LOG_LEVEL_ERROR, "Listening socket is in wrong state we "
						"terminate listener");
					break;
				}
			}

			/* wait - timeout -1 means wait indefinitely*/
			if (g_obj_wait(robjs, robjs_count, 0, 0, timeout) != 0)
			{
				/* error, should not get here */
				g_sleep(100);
			}

			if (g_is_wait_obj_set(term_obj)) /* termination called */
			{
				break;
			}

			/* some function must be processed by this thread */
			if (g_is_wait_obj_set(sync_obj))
			{
				g_reset_wait_obj(sync_obj);
				g_process_waiting_function(); /* run the function */
			}

			if (g_is_wait_obj_set(done_obj)) /* pro_done_event */
			{
				g_reset_wait_obj(done_obj);
				/* a process has died remove it from lists*/
				xrdp_listen_delete_done_pro(self);
			}

			/* Run the callback when accept() returns a new socket*/
			if (trans_check_wait_objs(self->listen_trans) != 0)
			{
				break;
			}
		}

		/* stop listening */
		trans_delete(self->listen_trans);
		self->listen_trans = 0;
		/* second loop to wait for all process threads to close */
		cont = 1;

		while (cont)
		{
			if (self->process_list->count == 0)
			{
				break;
			}

			timeout = -1;
			/* build the wait obj list */
			robjs_count = 0;
			robjs[robjs_count++] = sync_obj;
			robjs[robjs_count++] = done_obj;

			/* wait - timeout -1 means wait indefinitely*/
			if (g_obj_wait(robjs, robjs_count, 0, 0, timeout) != 0)
			{
				/* error, should not get here */
				g_sleep(100);
			}

			/* some function must be processed by this thread */
			if (g_is_wait_obj_set(sync_obj))
			{
				g_reset_wait_obj(sync_obj);
				g_process_waiting_function(); /* run the function that is waiting*/
			}

			if (g_is_wait_obj_set(done_obj)) /* pro_done_event */
			{
				g_reset_wait_obj(done_obj);
				xrdp_listen_delete_done_pro(self);
			}
		}
	}
	else
	{
		log_message(LOG_LEVEL_ERROR, "xrdp_listen_main_loop: listen error, possible port "
			"already in use");
	}

	self->status = -1;
	return 0;
}

#else

#include <winpr/crt.h>
#include <winpr/thread.h>

#include <freerdp/listener.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/signal.h>

void xrdp_peer_accepted(freerdp_listener* instance, freerdp_peer* client)
{
	HANDLE thread;
	xrdpProcess* process;
	printf("xrdp_peer_accepted\n");

	process = xrdp_process_create_ex(NULL, 0, (void*) client);

	thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) xrdp_process_main_thread, client, 0, NULL);
}

xrdpListener* xrdp_listen_create(void)
{
	freerdp_listener* listener;

	listener = freerdp_listener_new();
	listener->PeerAccepted = xrdp_peer_accepted;

	return (xrdpListener*) listener;
}

void xrdp_listen_delete(xrdpListener* self)
{
	freerdp_listener_free((freerdp_listener*) self);
}

int xrdp_listen_main_loop(xrdpListener* self)
{
	int i;
	int fds;
	int max_fds;
	int rcount;
	void* rfds[32];
	fd_set rfds_set;
	freerdp_listener* listener;

	ZeroMemory(rfds, sizeof(rfds));
	listener = (freerdp_listener*) self;

	listener->Open(listener, NULL, 3389);

	while (1)
	{
		rcount = 0;

		if (listener->GetFileDescriptor(listener, rfds, &rcount) != TRUE)
		{
			fprintf(stderr, "Failed to get FreeRDP file descriptor\n");
			break;
		}

		max_fds = 0;
		FD_ZERO(&rfds_set);

		for (i = 0; i < rcount; i++)
		{
			fds = (int)(long)(rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		if (max_fds == 0)
			break;

		if (select(max_fds + 1, &rfds_set, NULL, NULL, NULL) == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				fprintf(stderr, "select failed\n");
				break;
			}
		}

		if (listener->CheckFileDescriptor(listener) != TRUE)
		{
			fprintf(stderr, "Failed to check FreeRDP file descriptor\n");
			break;
		}
	}

	listener->Close(listener);

	return 0;
}

int xrdp_listen_set_startup_params(xrdpListener* self, struct xrdp_startup_params* startup_params)
{
	return 0;
}

#endif
