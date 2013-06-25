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
 * @file libscp_v1c_mng.c
 * @brief libscp version 1 client api code - session management
 * @author Simone Fedele
 *
 */

#include "libscp_v1c_mng.h"

#include <stdlib.h>
#include <stdio.h>

static enum SCP_CLIENT_STATES_E
_scp_v1c_mng_check_response(struct SCP_CONNECTION *c, struct SCP_SESSION *s);

/* client API */
/* 001 */
enum SCP_CLIENT_STATES_E
scp_v1c_mng_connect(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
	tui8 sz;
	tui32 size;

	init_stream(c->out_s, c->out_s->capacity);
	init_stream(c->in_s, c->in_s->capacity);

	size = 12 + 4 + g_strlen(s->hostname) + g_strlen(s->username) +
			g_strlen(s->password);

	if (s->addr_type == SCP_ADDRESS_TYPE_IPV4)
	{
		size = size + 4;
	}
	else
	{
		size = size + 16;
	}

	/* sending request */

	/* header */
	out_uint32_be(c->out_s, 1); /* version */
	out_uint32_be(c->out_s, size);
	out_uint16_be(c->out_s, SCP_COMMAND_SET_MANAGE);
	out_uint16_be(c->out_s, SCP_CMD_MNG_LOGIN);

	/* data */
	sz = g_strlen(s->username);
	Stream_Write_UINT8(c->out_s, sz);
	Stream_Write(c->out_s, s->username, sz);
	sz = g_strlen(s->password);
	Stream_Write_UINT8(c->out_s, sz);
	Stream_Write(c->out_s, s->password, sz);

	/* address */
	Stream_Write_UINT8(c->out_s, s->addr_type);

	if (s->addr_type == SCP_ADDRESS_TYPE_IPV4)
	{
		out_uint32_be(c->out_s, s->ipv4addr);
	}
	else
	{
		Stream_Write(c->out_s, s->ipv6addr, 16);
	}

	/* hostname */
	sz = g_strlen(s->hostname);
	Stream_Write_UINT8(c->out_s, sz);
	Stream_Write(c->out_s, s->hostname, sz);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: network error", __LINE__);
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	/* wait for response */
	return _scp_v1c_mng_check_response(c, s);
}

/* 004 */
enum SCP_CLIENT_STATES_E
scp_v1c_mng_get_session_list(struct SCP_CONNECTION *c, int *scount,
		struct SCP_DISCONNECTED_SESSION **s)
{
	tui32 version = 1;
	tui32 size = 12;
	tui16 cmd = SCP_CMD_MNG_LIST_REQ;       /* request session list */
	tui32 sescnt = 0;    /* total session number */
	tui32 sestmp = 0;    /* additional total session number */
	tui8 pktcnt = 0;     /* packet session count */
	tui32 totalcnt = 0;  /* session counter */
	tui8 continued = 0;  /* continue flag */
	int firstpkt = 1;    /* "first packet" flag */
	int idx;
	struct SCP_DISCONNECTED_SESSION *ds = 0;
	//   tui8 addr[16];

	init_stream(c->out_s, c->out_s->capacity);

	/* we request session list */
	out_uint32_be(c->out_s, version);                 /* version */
	out_uint32_be(c->out_s, size);                    /* size    */
	out_uint16_be(c->out_s, SCP_COMMAND_SET_MANAGE); /* cmdset  */
	out_uint16_be(c->out_s, cmd);                     /* cmd     */

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: network error", __LINE__);
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	do
	{
		/* then we wait for server response */
		init_stream(c->in_s, c->in_s->capacity);

		if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
		{
			log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: network error", __LINE__);
			return SCP_CLIENT_STATE_NETWORK_ERR;
		}

		in_uint32_be(c->in_s, version);

		if (version != 1)
		{
			log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: version error", __LINE__);
			return SCP_CLIENT_STATE_VERSION_ERR;
		}

		in_uint32_be(c->in_s, size);

		if (size < 12)
		{
			log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: size error", __LINE__);
			return SCP_CLIENT_STATE_SIZE_ERR;
		}

		init_stream(c->in_s, c->in_s->capacity);

		if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, size - 8))
		{
			log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: network error", __LINE__);
			return SCP_CLIENT_STATE_NETWORK_ERR;
		}

		in_uint16_be(c->in_s, cmd);

		if (cmd != SCP_COMMAND_SET_MANAGE)
		{
			log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: sequence error", __LINE__);
			return SCP_CLIENT_STATE_SEQUENCE_ERR;
		}

		in_uint16_be(c->in_s, cmd);

		if (cmd != SCP_CMD_MNG_LIST) /* session list */
		{
			log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: sequence error", __LINE__);
			return SCP_CLIENT_STATE_SEQUENCE_ERR;
		}

		if (firstpkt)
		{
			firstpkt = 0;
			in_uint32_be(c->in_s, sescnt);
			sestmp = sescnt;

			if (0 == sescnt)
			{
				/* return data... */
						(*scount) = sescnt;
						(*s) = NULL;

						LOG_DBG("[v1c_mng] end list - no session on TS");
						return SCP_CLIENT_STATE_LIST_OK;
			}

			ds = g_malloc(sizeof(struct SCP_DISCONNECTED_SESSION) * sescnt, 0);

			if (ds == 0)
			{
				log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: internal error", __LINE__);
				return SCP_CLIENT_STATE_INTERNAL_ERR;
			}
		}
		else
		{
			in_uint32_be(c->in_s, sestmp);
		}

		Stream_Read_UINT8(c->in_s, continued);
		Stream_Read_UINT8(c->in_s, pktcnt);

		for (idx = 0; idx < pktcnt; idx++)
		{
			in_uint32_be(c->in_s, (ds[totalcnt]).SID); /* session id */
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).type);
			in_uint16_be(c->in_s, (ds[totalcnt]).height);
			in_uint16_be(c->in_s, (ds[totalcnt]).width);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).bpp);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).idle_days);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).idle_hours);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).idle_minutes);

			in_uint16_be(c->in_s, (ds[totalcnt]).conn_year);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).conn_month);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).conn_day);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).conn_hour);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).conn_minute);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).addr_type);

			if ((ds[totalcnt]).addr_type == SCP_ADDRESS_TYPE_IPV4)
			{
				in_uint32_be(c->in_s, (ds[totalcnt]).ipv4addr);
			}

			if ((ds[totalcnt]).addr_type == SCP_ADDRESS_TYPE_IPV6)
			{
				Stream_Read(c->in_s, (ds[totalcnt]).ipv6addr, 16);
			}

			totalcnt++;
		}
	}
	while (continued);

	/* return data... */
	(*scount) = sescnt;
	(*s) = ds;

	LOG_DBG("[v1c_mng] end list");
	return SCP_CLIENT_STATE_LIST_OK;
}

static enum SCP_CLIENT_STATES_E
_scp_v1c_mng_check_response(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
	tui32 version;
	tui32 size;
	tui16 cmd;
	tui8 dim;
	char buf[257];

	init_stream(c->in_s, c->in_s->capacity);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: network error", __LINE__);
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	in_uint32_be(c->in_s, version);

	if (version != 1)
	{
		log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: version error", __LINE__);
		return SCP_CLIENT_STATE_VERSION_ERR;
	}

	in_uint32_be(c->in_s, size);

	init_stream(c->in_s, c->in_s->capacity);

	/* read the rest of the packet */
	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, size - 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: network error", __LINE__);
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	in_uint16_be(c->in_s, cmd);

	if (cmd != SCP_COMMAND_SET_MANAGE)
	{
		log_message(LOG_LEVEL_WARNING, "[v1c_mng:%d] connection aborted: sequence error", __LINE__);
		return SCP_CLIENT_STATE_SEQUENCE_ERR;
	}

	in_uint16_be(c->in_s, cmd);

	if (cmd == SCP_CMD_MNG_LOGIN_ALLOW) /* connection ok */
	{
		log_message(LOG_LEVEL_INFO, "[v1c_mng:%d] connection ok", __LINE__);
		return SCP_CLIENT_STATE_OK;
	}
	else if (cmd == SCP_CMD_MNG_LOGIN_DENY) /* connection denied */
	{
		Stream_Read_UINT8(c->in_s, dim);
		buf[dim] = '\0';
		Stream_Read(c->in_s, buf, dim);
		scp_session_set_errstr(s, buf);

		log_message(LOG_LEVEL_INFO, "[v1c_mng:%d] connection denied: %s", __LINE__ , s->errstr);
		return SCP_CLIENT_STATE_CONNECTION_DENIED;
	}

	log_message(LOG_LEVEL_WARNING, "[v1c-mng:%d] connection aborted: sequence error", __LINE__);
	return SCP_CLIENT_STATE_SEQUENCE_ERR;
}
