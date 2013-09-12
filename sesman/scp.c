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
 */

/**
 *
 * @file scp.c
 * @brief scp (sesman control protocol) common code
 *        scp (sesman control protocol) common code
 *        This code controls which version is being used and starts the
 *        appropriate process
 * @author Jay Sorg, Simone Fedele
 *
 */

#include "sesman.h"

extern int g_thread_sck; /* in thread.c */
extern struct config_sesman *g_cfg; /* in sesman.c */

void scp_server_process(SCP_CONNECTION *c, SCP_SESSION *s)
{
	int display = 0;
	LONG_PTR data;
	xrdpSessionItem *s_item;
	int errorcode = 0;

	data = auth_userpass(s->username, s->password, &errorcode);

	if (data)
	{
		s_item = session_get_bydata(s->username, s->width, s->height, s->bpp, s->type);

		if (s_item != 0)
		{
			display = s_item->display;

			if (0 != s->client_ip)
			{
				log_message(LOG_LEVEL_INFO, "++ reconnected session: username %s, "
					"display :%d.0, session_pid %d, ip %s", s->username, display,
						s_item->pid, s->client_ip);
			}
			else
			{
				log_message(LOG_LEVEL_INFO, "++ reconnected session: username %s, "
					"display :%d.0, session_pid %d", s->username, display, s_item->pid);
			}

			session_reconnect(display, s->username);
			auth_end(data);
			/* don't set data to null here */
		}
		else
		{
			LOG_DBG("pre auth");

			if (1 == access_login_allowed(s->username))
			{
				if (0 != s->client_ip)
				{
					log_message(LOG_LEVEL_INFO, "++ created session (access granted): "
						"username %s, ip %s", s->username, s->client_ip);
				}
				else
				{
					log_message(LOG_LEVEL_INFO, "++ created session (access granted): "
						"username %s", s->username);
				}

				if (s->type == SCP_SESSION_TYPE_XRDP)
				{
					log_message(LOG_LEVEL_INFO, "starting X11rdp session...");
					display = session_start(s->width, s->height, s->bpp, s->username,
							s->password, data, SESMAN_SESSION_TYPE_XRDP, s->domain,
							s->program, s->directory, s->client_ip);
				}
			}
			else
			{
				display = 0;
			}
		}

		if (display == 0)
		{
			auth_end(data);
			scp_server_deny_connection(c);
		}
		else
		{
			scp_server_allow_connection(c, display);
		}
	}
	else
	{
		scp_server_deny_connection(c);
	}
}

void* scp_process_start(void *sck)
{
	SCP_CONNECTION scon;
	SCP_SESSION *sdata;

	/* making a local copy of the socket (it's on the stack) */
	/* probably this is just paranoia                        */
	scon.in_sck = g_thread_sck;
	LOG_DBG("started scp thread on socket %d", scon.in_sck);

	/* unlocking g_thread_sck */
	lock_socket_release();

	scon.in_s = Stream_New(NULL, 8192);
	scon.out_s = Stream_New(NULL, 8192);

	switch (scp_server_accept(&scon, &(sdata), 0))
	{
		case SCP_SERVER_STATE_OK:
			scp_server_process(&scon, sdata);
			break;

		case SCP_SERVER_STATE_VERSION_ERR:
			/* an unknown scp version was requested, so we shut down the */
			/* connection (and log the fact)                             */
			log_message(LOG_LEVEL_WARNING, "unknown protocol version specified. connection refused.");
			break;

		case SCP_SERVER_STATE_NETWORK_ERR:
			log_message(LOG_LEVEL_WARNING, "libscp network error.");
			break;

		case SCP_SERVER_STATE_SEQUENCE_ERR:
			log_message(LOG_LEVEL_WARNING, "libscp sequence error.");
			break;

		case SCP_SERVER_STATE_INTERNAL_ERR:
			/* internal error occurred (eg. malloc() error, ecc.) */
			log_message(LOG_LEVEL_ERROR, "libscp internal error occurred.");
			break;

		default:
			log_message(LOG_LEVEL_ALWAYS, "unknown return from scp_vXs_accept()");
	}

	g_tcp_close(scon.in_sck);

	Stream_Free(scon.in_s, TRUE);
	Stream_Free(scon.out_s, TRUE);

	return 0;
}
