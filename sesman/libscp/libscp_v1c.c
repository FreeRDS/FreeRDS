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
 * @file libscp_v1c.c
 * @brief libscp version 1 client api code
 * @author Simone Fedele
 *
 */

#include "libscp_v1c.h"

#include <stdlib.h>
#include <stdio.h>

static enum SCP_CLIENT_STATES_E _scp_v1c_check_response(struct SCP_CONNECTION *c, struct SCP_SESSION *s);

/* client API */
/* 001 */
enum SCP_CLIENT_STATES_E scp_v1c_connect(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
	BYTE sz;
	UINT32 size;

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	size = 19 + 17 + 4 + g_strlen(s->hostname) + g_strlen(s->username) +
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
	Stream_Write_UINT32_BE(c->out_s, 1); /* version */
	Stream_Write_UINT32_BE(c->out_s, size);
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT);
	Stream_Write_UINT16_BE(c->out_s, 1);

	/* body */
	Stream_Write_UINT8(c->out_s, s->type);
	Stream_Write_UINT16_BE(c->out_s, s->height);
	Stream_Write_UINT16_BE(c->out_s, s->width);
	Stream_Write_UINT8(c->out_s, s->bpp);
	Stream_Write_UINT8(c->out_s, s->rsr);
	Stream_Write(c->out_s, s->locale, 17);
	Stream_Write_UINT8(c->out_s, s->addr_type);

	if (s->addr_type == SCP_ADDRESS_TYPE_IPV4)
	{
		Stream_Write_UINT32_BE(c->out_s, s->ipv4addr);
	}
	else if (s->addr_type == SCP_ADDRESS_TYPE_IPV6)
	{
		Stream_Write(c->out_s, s->ipv6addr, 16);
	}

	sz = g_strlen(s->hostname);
	Stream_Write_UINT8(c->out_s, sz);
	Stream_Write(c->out_s, s->hostname, sz);
	sz = g_strlen(s->username);
	Stream_Write_UINT8(c->out_s, sz);
	Stream_Write(c->out_s, s->username, sz);
	sz = g_strlen(s->password);
	Stream_Write_UINT8(c->out_s, sz);
	Stream_Write(c->out_s, s->password, sz);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	/* wait for response */
	return _scp_v1c_check_response(c, s);
}

/* 004 */
enum SCP_CLIENT_STATES_E scp_v1c_resend_credentials(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
	BYTE sz;
	UINT32 size;

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	size = 12 + 2 + g_strlen(s->username) + g_strlen(s->password);

	/* sending request */
	/* header */
	Stream_Write_UINT32_BE(c->out_s, 1); /* version */
	Stream_Write_UINT32_BE(c->out_s, size);
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT);
	Stream_Write_UINT16_BE(c->out_s, 4);

	/* body */
	sz = g_strlen(s->username);
	Stream_Write_UINT8(c->out_s, sz);
	Stream_Write(c->out_s, s->username, sz);
	sz = g_strlen(s->password);
	Stream_Write_UINT8(c->out_s, sz);
	Stream_Write(c->out_s, s->password, sz);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	/* wait for response */
	return _scp_v1c_check_response(c, s);
}

/* 021 */
enum SCP_CLIENT_STATES_E scp_v1c_pwd_change(struct SCP_CONNECTION *c, char *newpass);
/* 022 */
enum SCP_CLIENT_STATES_E scp_v1c_pwd_change_cancel(struct SCP_CONNECTION *c);

/* 041 */
enum SCP_CLIENT_STATES_E scp_v1c_get_session_list(struct SCP_CONNECTION *c, int *scount, struct SCP_DISCONNECTED_SESSION **s)
{
	UINT32 version = 1;
	UINT32 size = 12;
	UINT16 cmd = 41;
	UINT32 sescnt = 0;    /* total session number */
	UINT32 sestmp = 0;    /* additional total session number */
	BYTE pktcnt = 0;     /* packet session count */
	UINT32 totalcnt = 0;  /* session counter */
	BYTE continued = 0;  /* continue flag */
	int firstpkt = 1;    /* "first packet" flag */
	int idx;
	struct SCP_DISCONNECTED_SESSION *ds = 0;

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	/* we request session list */
	Stream_Write_UINT32_BE(c->out_s, version);                 /* version */
	Stream_Write_UINT32_BE(c->out_s, size);                    /* size    */
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT); /* cmdset  */
	Stream_Write_UINT16_BE(c->out_s, cmd);                     /* cmd     */

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	do
	{
		/* then we wait for server response */
		Stream_Clear(c->in_s);
		Stream_SetPosition(c->in_s, 0);

		if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
		{
			return SCP_CLIENT_STATE_NETWORK_ERR;
		}

		Stream_Read_UINT32_BE(c->in_s, version);

		if (version != 1)
		{
			return SCP_CLIENT_STATE_VERSION_ERR;
		}

		Stream_Read_UINT32_BE(c->in_s, size);

		if (size < 12)
		{
			return SCP_CLIENT_STATE_SIZE_ERR;
		}

		Stream_Clear(c->in_s);
		Stream_SetPosition(c->in_s, 0);

		if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, size - 8))
		{
			return SCP_CLIENT_STATE_NETWORK_ERR;
		}

		Stream_Read_UINT16_BE(c->in_s, cmd);

		if (cmd != SCP_COMMAND_SET_DEFAULT)
		{
			return SCP_CLIENT_STATE_SEQUENCE_ERR;
		}

		Stream_Read_UINT16_BE(c->in_s, cmd);

		if (cmd != 42)
		{
			return SCP_CLIENT_STATE_SEQUENCE_ERR;
		}

		if (firstpkt)
		{
			firstpkt = 0;
			Stream_Read_UINT32_BE(c->in_s, sescnt);
			sestmp = sescnt;

			ds = g_malloc(sizeof(struct SCP_DISCONNECTED_SESSION) * sescnt, 0);

			if (ds == 0)
			{
				return SCP_CLIENT_STATE_INTERNAL_ERR;
			}
		}
		else
		{
			Stream_Read_UINT32_BE(c->in_s, sestmp);
		}

		Stream_Read_UINT8(c->in_s, continued);
		Stream_Read_UINT8(c->in_s, pktcnt);

		for (idx = 0; idx < pktcnt; idx++)
		{
			Stream_Read_UINT32_BE(c->in_s, (ds[totalcnt]).SID); /* session id */
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).type);
			Stream_Read_UINT16_BE(c->in_s, (ds[totalcnt]).height);
			Stream_Read_UINT16_BE(c->in_s, (ds[totalcnt]).width);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).bpp);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).idle_days);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).idle_hours);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).idle_minutes);

			Stream_Read_UINT16_BE(c->in_s, (ds[totalcnt]).conn_year);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).conn_month);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).conn_day);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).conn_hour);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).conn_minute);
			Stream_Read_UINT8(c->in_s, (ds[totalcnt]).addr_type);

			if ((ds[totalcnt]).addr_type == SCP_ADDRESS_TYPE_IPV4)
			{
				Stream_Read_UINT32_BE(c->in_s, (ds[totalcnt]).ipv4addr);
			}
			else if ((ds[totalcnt]).addr_type == SCP_ADDRESS_TYPE_IPV6)
			{
				Stream_Read(c->in_s, (ds[totalcnt]).ipv6addr, 16);
			}

			totalcnt++;
		}
	}
	while (continued);

	printf("fine\n");
	/* return data... */
	(*scount) = sescnt;
	(*s) = ds;

	return SCP_CLIENT_STATE_LIST_OK;
}

/* 043 */
enum SCP_CLIENT_STATES_E scp_v1c_select_session(struct SCP_CONNECTION *c, struct SCP_SESSION *s, SCP_SID sid)
{
	UINT32 version = 1;
	UINT32 size = 16;
	UINT16 cmd = 43;

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	/* sending our selection */
	Stream_Write_UINT32_BE(c->out_s, version);                 /* version */
	Stream_Write_UINT32_BE(c->out_s, size);                    /* size    */
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT); /* cmdset  */
	Stream_Write_UINT16_BE(c->out_s, cmd);                     /* cmd     */

	Stream_Write_UINT32_BE(c->out_s, sid);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	/* waiting for response.... */
	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, version);

	if (version != 1)
	{
		return SCP_CLIENT_STATE_VERSION_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	if (size < 12)
	{
		return SCP_CLIENT_STATE_SIZE_ERR;
	}

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	/* read the rest of the packet */
	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, size - 8))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != SCP_COMMAND_SET_DEFAULT)
	{
		return SCP_CLIENT_STATE_SEQUENCE_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != 46)
	{
		return SCP_CLIENT_STATE_SEQUENCE_ERR;
	}

	/* session display */
	Stream_Read_UINT16_BE(c->in_s, (s->display));
	/*we don't need to return any data other than the display */
	/*because we already sent that                            */

	return SCP_CLIENT_STATE_OK;
}

/* 044 */
enum SCP_CLIENT_STATES_E scp_v1c_select_session_cancel(struct SCP_CONNECTION *c)
{
	UINT32 version = 1;
	UINT32 size = 12;
	UINT16 cmd = 44;

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	/* sending our selection */
	Stream_Write_UINT32_BE(c->out_s, version);                 /* version */
	Stream_Write_UINT32_BE(c->out_s, size);                    /* size    */
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT); /* cmdset  */
	Stream_Write_UINT16_BE(c->out_s, cmd);                     /* cmd     */

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	return SCP_CLIENT_STATE_END;
}

static enum SCP_CLIENT_STATES_E _scp_v1c_check_response(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
	UINT32 version;
	UINT32 size;
	UINT16 cmd;
	UINT16 dim;

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, version);

	if (version != 1)
	{
		return SCP_CLIENT_STATE_VERSION_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	/* read the rest of the packet */
	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, size - 8))
	{
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != SCP_COMMAND_SET_DEFAULT)
	{
		return SCP_CLIENT_STATE_SEQUENCE_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd == 2) /* connection denied */
	{
		Stream_Read_UINT16_BE(c->in_s, dim);

		if (s->errstr != 0)
		{
			free(s->errstr);
		}

		s->errstr = g_malloc(dim + 1, 0);

		if (s->errstr == 0)
		{
			return SCP_CLIENT_STATE_INTERNAL_ERR;
		}

		Stream_Read(c->in_s, s->errstr, dim);
		(s->errstr)[dim] = '\0';

		return SCP_CLIENT_STATE_CONNECTION_DENIED;
	}
	else if (cmd == 3) /* resend usr/pwd */
	{
		Stream_Read_UINT16_BE(c->in_s, dim);

		if (s->errstr != 0)
		{
			free(s->errstr);
		}

		s->errstr = g_malloc(dim + 1, 0);

		if (s->errstr == 0)
		{
			return SCP_CLIENT_STATE_INTERNAL_ERR;
		}

		Stream_Read(c->in_s, s->errstr, dim);
		(s->errstr)[dim] = '\0';

		return SCP_CLIENT_STATE_RESEND_CREDENTIALS;
	}
	else if (cmd == 20) /* password change */
	{
		Stream_Read_UINT16_BE(c->in_s, dim);

		if (s->errstr != 0)
		{
			free(s->errstr);
		}

		s->errstr = g_malloc(dim + 1, 0);

		if (s->errstr == 0)
		{
			return SCP_CLIENT_STATE_INTERNAL_ERR;
		}

		Stream_Read(c->in_s, s->errstr, dim);
		(s->errstr)[dim] = '\0';

		return SCP_CLIENT_STATE_PWD_CHANGE_REQ;
	}
	else if (cmd == 30) /* display */
	{
		Stream_Read_UINT16_BE(c->in_s, s->display);

		return SCP_CLIENT_STATE_OK;
	}
	else if (cmd == 40) /* session list */
	{
		return SCP_CLIENT_STATE_SESSION_LIST;
	}

	return SCP_CLIENT_STATE_SEQUENCE_ERR;
}
