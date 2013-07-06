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

void* scp_process_start(void *sck)
{
	struct SCP_CONNECTION scon;
	struct SCP_SESSION *sdata;

	/* making a local copy of the socket (it's on the stack) */
	/* probably this is just paranoia                        */
	scon.in_sck = g_thread_sck;
	LOG_DBG("started scp thread on socket %d", scon.in_sck);

	/* unlocking g_thread_sck */
	lock_socket_release();

	scon.in_s = Stream_New(NULL, 8192);
	scon.out_s = Stream_New(NULL, 8192);

	switch (scp_vXs_accept(&scon, &(sdata)))
	{
		case SCP_SERVER_STATE_OK:
			scp_v0_process(&scon, sdata);
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
