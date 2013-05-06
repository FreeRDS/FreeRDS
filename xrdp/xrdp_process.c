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
 * main rdp process
 */

#include "xrdp.h"

struct xrdp_process
{
	int status;
	struct trans* server_trans; /* in tcp server mode */
	tbus self_term_event;
	struct xrdp_session* session;
	/* create these when up and running */
	struct xrdp_wm* wm;
	tbus done_event;
	int session_id;
};

static int g_session_id = 0;

/*****************************************************************************/
/* always called from xrdp_listen thread */
xrdpProcess* xrdp_process_create(xrdpListener *owner, tbus done_event)
{
	int pid;
	xrdpProcess* self;
	char event_name[256];

	self = (xrdpProcess*) g_malloc(sizeof(xrdpProcess), 1);

	self->done_event = done_event;
	g_session_id++;
	self->session_id = g_session_id;
	pid = g_getpid();
	g_snprintf(event_name, 255, "xrdp_%8.8x_process_self_term_event_%8.8x", pid, self->session_id);
	self->self_term_event = g_create_wait_obj(event_name);

	return self;
}

/*****************************************************************************/
void xrdp_process_delete(xrdpProcess *self)
{
	if (self == 0)
	{
		return;
	}

	g_delete_wait_obj(self->self_term_event);
	libxrdp_exit(self->session);
	xrdp_wm_delete(self->wm);
	trans_delete(self->server_trans);
	g_free(self);
}

/*****************************************************************************/
static int xrdp_process_loop(xrdpProcess *self)
{
	int rv;

	rv = 0;

	if (self->session != 0)
	{
		rv = libxrdp_process_data(self->session);
	}

	if ((self->wm == 0) && (self->session->up_and_running) && (rv == 0))
	{
		DEBUG(("calling xrdp_wm_init and creating wm"));
		self->wm = xrdp_wm_create(self, self->session->client_info);
		/* at this point the wm(window manager) is create and wm::login_mode is
		 zero and login_mode_event is set so xrdp_wm_init should be called by
		 xrdp_wm_check_wait_objs */
	}

	return rv;
}

/*****************************************************************************/
/* returns boolean */
/* this is so libxrdp.so can known when to quit looping */
static int xrdp_is_term(void)
{
	return g_is_term();
}

/*****************************************************************************/
static int xrdp_process_mod_end(xrdpProcess *self)
{
	if (self->wm != 0)
	{
		if (self->wm->mm != 0)
		{
			if (self->wm->mm->mod != 0)
			{
				if (self->wm->mm->mod->mod_end != 0)
				{
					return self->wm->mm->mod->mod_end(self->wm->mm->mod);
				}
			}
		}
	}

	return 0;
}

/*****************************************************************************/
static int xrdp_process_data_in(struct trans* self)
{
	xrdpProcess *pro;

	DEBUG(("xrdp_process_data_in"));
	pro = (xrdpProcess*) (self->callback_data);

	if (xrdp_process_loop(pro) != 0)
	{
		return 1;
	}

	return 0;
}

int APP_CC xrdp_process_get_status(xrdpProcess* self)
{
	return self->status;
}

struct xrdp_session* APP_CC xrdp_process_get_session(xrdpProcess* self)
{
	return self->session;
}

int APP_CC xrdp_process_get_session_id(xrdpProcess* self)
{
	return self->session_id;
}

struct xrdp_wm* APP_CC xrdp_process_get_wm(xrdpProcess* self)
{
	return self->wm;
}

tbus APP_CC xrdp_process_get_term_event(xrdpProcess* self)
{
	return self->self_term_event;
}

void APP_CC xrdp_process_set_transport(xrdpProcess* self, struct trans* transport)
{
	self->server_trans = transport;
}

/*****************************************************************************/
int xrdp_process_main_loop(xrdpProcess *self)
{
	int robjs_count;
	int wobjs_count;
	int cont;
	int timeout = 0;
	tbus robjs[32];
	tbus wobjs[32];
	tbus term_obj;

	DEBUG(("xrdp_process_main_loop"));
	self->status = 1;
	self->server_trans->trans_data_in = xrdp_process_data_in;
	self->server_trans->callback_data = self;
	self->session = libxrdp_init((tbus) self, self->server_trans);
	/* this callback function is in xrdp_wm.c */
	self->session->callback = callback;
	/* this function is just above */
	self->session->is_term = xrdp_is_term;

	if (libxrdp_process_incomming(self->session) == 0)
	{
		term_obj = g_get_term_event();
		cont = 1;

		while (cont)
		{
			/* build the wait obj list */
			timeout = -1;
			robjs_count = 0;
			wobjs_count = 0;
			robjs[robjs_count++] = term_obj;
			robjs[robjs_count++] = self->self_term_event;
			xrdp_wm_get_wait_objs(self->wm, robjs, &robjs_count, wobjs, &wobjs_count, &timeout);
			trans_get_wait_objs(self->server_trans, robjs, &robjs_count);

			/* wait */
			if (g_obj_wait(robjs, robjs_count, wobjs, wobjs_count, timeout) != 0)
			{
				/* error, should not get here */
				g_sleep(100);
			}

			if (g_is_wait_obj_set(term_obj)) /* term */
			{
				break;
			}

			if (g_is_wait_obj_set(self->self_term_event))
			{
				break;
			}

			if (xrdp_wm_check_wait_objs(self->wm) != 0)
			{
				break;
			}

			if (trans_check_wait_objs(self->server_trans) != 0)
			{
				break;
			}
		}
		/* send disconnect message if possible */
		libxrdp_disconnect(self->session);
	} else
	{
		g_writeln("xrdp_process_main_loop: libxrdp_process_incomming failed");
	}
	/* Run end in module */
	xrdp_process_mod_end(self);
	libxrdp_exit(self->session);
	self->session = 0;
	self->status = -1;
	g_set_wait_obj(self->done_event);
	return 0;
}
