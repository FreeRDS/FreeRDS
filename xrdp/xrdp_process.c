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

static int g_session_id = 0;

#define NEW_XRDP_PROCESS	1

#ifndef NEW_XRDP_PROCESS

struct xrdp_process
{
	int status;
	struct trans* server_trans; /* in tcp server mode */
	tbus self_term_event;
	xrdpSession* session;
	/* create these when up and running */
	xrdpWm* wm;
	tbus done_event;
	int session_id;
};

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

xrdpProcess* xrdp_process_create_ex(xrdpListener* owner, tbus done_event, void* transport)
{
	return xrdp_process_create(owner, done_event);
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

xrdpSession* APP_CC xrdp_process_get_session(xrdpProcess* self)
{
	return self->session;
}

int APP_CC xrdp_process_get_session_id(xrdpProcess* self)
{
	return self->session_id;
}

xrdpWm* APP_CC xrdp_process_get_wm(xrdpProcess* self)
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

void* xrdp_process_main_thread(void* arg)
{
	return NULL;
}

#else

#include <winpr/crt.h>
#include <winpr/file.h>
#include <winpr/path.h>
#include <winpr/thread.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/signal.h>

#include "makecert.h"

struct xrdp_process
{
	rdpContext context;

	int status;
	int session_id;
	BOOL activated;
	tbus done_event;
	tbus term_event;
	xrdpWm* wm;
	xrdpSession* session;
	xrdpClientInfo* info;
};

void xrdp_peer_context_new(freerdp_peer* client, xrdpProcess* context)
{
	context->info = (xrdpClientInfo*) malloc(sizeof(xrdpClientInfo));

	if (context->info)
	{
		ZeroMemory(context->info, sizeof(xrdpClientInfo));
		context->info->size = sizeof(xrdpClientInfo);
	}

	context->session = (xrdpSession*) malloc(sizeof(xrdpSession));

	if (context->session)
	{
		ZeroMemory(context->session, sizeof(xrdpSession));
		context->session->client_info = context->info;

		context->session->context = (rdpContext*) context;
		context->session->client = client;
	}

	context->status = 1;
}

void xrdp_peer_context_free(freerdp_peer* client, xrdpProcess* context)
{

}

xrdpProcess* xrdp_process_create(xrdpListener* owner, tbus done_event)
{
	return NULL;
}

xrdpProcess* xrdp_process_create_ex(xrdpListener* owner, tbus done_event, void* transport)
{
	int pid;
	xrdpProcess* xfp;
	freerdp_peer* client;
	char event_name[256];

	client = (freerdp_peer*) transport;

	client->context_size = sizeof(xrdpProcess);
	client->ContextNew = (psPeerContextNew) xrdp_peer_context_new;
	client->ContextFree = (psPeerContextFree) xrdp_peer_context_free;
	freerdp_peer_context_new(client);

	xfp = (xrdpProcess*) client->context;

	xfp->done_event = done_event;
	g_session_id++;
	xfp->session_id = g_session_id;
	pid = g_getpid();
	g_snprintf(event_name, 255, "xrdp_%8.8x_process_self_term_event_%8.8x", pid, xfp->session_id);
	xfp->term_event = g_create_wait_obj(event_name);

	return xfp;
}

void xrdp_process_delete(xrdpProcess* self)
{

}

int xrdp_process_get_status(xrdpProcess* self)
{
	return self->status;
}

tbus xrdp_process_get_term_event(xrdpProcess* self)
{
	return self->term_event;
}

xrdpSession* xrdp_process_get_session(xrdpProcess* self)
{
	return self->session;
}

int xrdp_process_get_session_id(xrdpProcess* self)
{
	return self->session_id;
}

xrdpWm* xrdp_process_get_wm(xrdpProcess* self)
{
	return self->wm;
}

void xrdp_process_set_transport(xrdpProcess* self, struct trans* transport)
{

}

int xrdp_process_main_loop(xrdpProcess* self)
{
	return 0;
}

BOOL xrdp_peer_capabilities(freerdp_peer* client)
{
	return TRUE;
}

BOOL xrdp_peer_post_connect(freerdp_peer* client)
{
	xrdpProcess* xfp;

	xfp = (xrdpProcess*) client->context;

	fprintf(stderr, "Client %s is activated", client->hostname);
	if (client->settings->AutoLogonEnabled)
	{
		fprintf(stderr, " and wants to login automatically as %s\\%s",
			client->settings->Domain ? client->settings->Domain : "",
			client->settings->Username);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "Client requested desktop: %dx%dx%d\n",
		client->settings->DesktopWidth, client->settings->DesktopHeight, client->settings->ColorDepth);

	client->update->DesktopResize(client->update->context);

	return TRUE;
}

BOOL xrdp_peer_activate(freerdp_peer* client)
{
	xrdpProcess* xfp = (xrdpProcess*) client->context;

	xfp->activated = TRUE;

	return TRUE;
}

const char* makecert_argv[4] =
{
	"makecert",
	"-rdp",
	"-live",
	"-silent"
};

int makecert_argc = (sizeof(makecert_argv) / sizeof(char*));

int xrdp_generate_certificate(rdpSettings* settings)
{
	char* server_file_path;
	MAKECERT_CONTEXT* context;

	server_file_path = GetCombinedPath(settings->ConfigPath, "server");

	if (!PathFileExistsA(server_file_path))
		CreateDirectoryA(server_file_path, 0);

	settings->CertificateFile = GetCombinedPath(server_file_path, "server.crt");
	settings->PrivateKeyFile = GetCombinedPath(server_file_path, "server.key");

	if ((!PathFileExistsA(settings->CertificateFile)) ||
			(!PathFileExistsA(settings->PrivateKeyFile)))
	{
		context = makecert_context_new();

		makecert_context_process(context, makecert_argc, (char**) makecert_argv);

		makecert_context_set_output_file_name(context, "server");

		if (!PathFileExistsA(settings->CertificateFile))
			makecert_context_output_certificate_file(context, server_file_path);

		if (!PathFileExistsA(settings->PrivateKeyFile))
			makecert_context_output_private_key_file(context, server_file_path);

		makecert_context_free(context);
	}

	free(server_file_path);

	return 0;
}

void xrdp_input_synchronize_event(rdpInput* input, UINT32 flags)
{

}

void xrdp_input_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	xrdpProcess* xfp = (xrdpProcess*) input->context;
	xrdp_wm_key(xfp->wm, flags, code);
}

void xrdp_input_unicode_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{

}

void xrdp_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	xrdpProcess* xfp = (xrdpProcess*) input->context;
	xrdp_wm_process_input_mouse(xfp->wm, flags, x, y);
}

void xrdp_input_extended_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	xrdpProcess* xfp = (xrdpProcess*) input->context;
	xrdp_wm_process_input_mouse(xfp->wm, flags, x, y);
}

void xrdp_input_register_callbacks(rdpInput* input)
{
	input->SynchronizeEvent = xrdp_input_synchronize_event;
	input->KeyboardEvent = xrdp_input_keyboard_event;
	input->UnicodeKeyboardEvent = xrdp_input_unicode_keyboard_event;
	input->MouseEvent = xrdp_input_mouse_event;
	input->ExtendedMouseEvent = xrdp_input_extended_mouse_event;
}

void* xrdp_process_main_thread(void* arg)
{
	int i;
	int fds;
	int max_fds;
	int rcount;
	int wcount;
	int robjc;
	int wobjc;
	int entries;
	long term_obj;
	long robjs[32];
	long wobjs[32];
	int itimeout;
	void* rfds[32];
	void* wfds[32];
	fd_set rfds_set;
	xrdpProcess* xfp;
	int bytesPerPixel;
	rdpSettings* settings;
	struct timeval timeout;
	freerdp_peer* client = (freerdp_peer*) arg;

	ZeroMemory(rfds, sizeof(rfds));
	ZeroMemory(wfds, sizeof(wfds));
	ZeroMemory(&timeout, sizeof(struct timeval));

	fprintf(stderr, "We've got a client %s\n", client->hostname);

	xfp = (xrdpProcess*) client->context;
	settings = client->settings;

	xrdp_generate_certificate(settings);

	client->Capabilities = xrdp_peer_capabilities;
	client->PostConnect = xrdp_peer_post_connect;
	client->Activate = xrdp_peer_activate;

	client->Initialize(client);

	xfp->info->bpp = settings->ColorDepth;
	xfp->info->width = settings->DesktopWidth;
	xfp->info->height = settings->DesktopHeight;
	xfp->info->build = settings->ClientBuild;
	xfp->info->keylayout = settings->KeyboardLayout;

	bytesPerPixel = (xfp->info->bpp + 7) / 8;

	entries = settings->BitmapCacheV2CellInfo[0].numEntries;
	entries = MIN(entries, 2000);
	xfp->info->cache1_entries = entries;
	xfp->info->cache1_size = 256 * bytesPerPixel;

	entries = settings->BitmapCacheV2CellInfo[1].numEntries;
	entries = MIN(entries, 2000);
	xfp->info->cache2_entries = entries;
	xfp->info->cache2_size = 1024 * bytesPerPixel;

	entries = settings->BitmapCacheV2CellInfo[1].numEntries;
	entries = MIN(entries, 2000);
	xfp->info->cache3_entries = entries;
	xfp->info->cache3_size = 4096 * bytesPerPixel;

	xfp->info->use_bitmap_comp = 1;
	xfp->info->bitmap_cache_version = 2;
	xfp->info->bitmap_cache_persist_enable = 0;
	xfp->info->pointer_cache_entries = settings->PointerCacheSize;

	xfp->info->offscreen_support_level = settings->OffscreenSupportLevel;
	xfp->info->offscreen_cache_size = settings->OffscreenCacheSize * 1024;
	xfp->info->offscreen_cache_entries = settings->OffscreenCacheEntries;

	CopyMemory(xfp->info->orders, settings->OrderSupport, 32);

	if (settings->Username)
		strcpy(xfp->info->username, settings->Username);

	if (settings->Password)
		strcpy(xfp->info->password, settings->Password);

	xfp->wm = xrdp_wm_create(xfp, xfp->info);

	xfp->session->callback = callback;
	xrdp_input_register_callbacks(client->input);

	term_obj = g_get_term_event();

	while (1)
	{
		rcount = 0;
		wcount = 0;

		robjc = 0;
		wobjc = 0;
		itimeout = 0;

		if (client->GetFileDescriptor(client, rfds, &rcount) != TRUE)
		{
			fprintf(stderr, "Failed to get FreeRDP file descriptor\n");
			break;
		}

		if (xfp->activated)
		{
			xrdp_wm_get_wait_objs(xfp->wm, robjs, &robjc, wobjs, &wobjc, &itimeout);
		}

		robjs[robjc++] = term_obj;
		robjs[robjc++] = xfp->term_event;

		max_fds = 0;
		FD_ZERO(&rfds_set);

		for (i = 0; i < rcount; i++)
		{
			fds = (int)(long)(rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		for (i = 0; i < robjc; i++)
		{
			fds = robjs[i];

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		if (max_fds == 0)
			break;

		timeout.tv_sec = 0;
		timeout.tv_usec = 100;

		if (select(max_fds + 1, &rfds_set, NULL, NULL, &timeout) == -1)
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

		if (client->CheckFileDescriptor(client) != TRUE)
		{
			fprintf(stderr, "Failed to check freerdp file descriptor\n");
			break;
		}

		if (xfp->activated)
		{
			if (xrdp_wm_check_wait_objs(xfp->wm) != 0)
			{
				break;
			}
		}

		if (g_is_wait_obj_set(term_obj))
		{
			break;
		}

		if (g_is_wait_obj_set(xfp->term_event))
		{
			break;
		}
	}

	fprintf(stderr, "Client %s disconnected.\n", client->hostname);

	client->Disconnect(client);

	freerdp_peer_context_free(client);
	freerdp_peer_free(client);

	return NULL;
}

#endif
