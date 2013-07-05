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
 * @file libscp_v1s_mng.c
 * @brief libscp version 1 server api code - session management
 * @author Simone Fedele
 *
 */

#ifndef LIBSCP_V1S_MNG_C
#define LIBSCP_V1S_MNG_C

#include "libscp_v1s_mng.h"

static enum SCP_SERVER_STATES_E _scp_v1s_mng_check_response(struct SCP_CONNECTION *c, struct SCP_SESSION *s);

/* server API */
enum SCP_SERVER_STATES_E scp_v1s_mng_accept(struct SCP_CONNECTION *c, struct SCP_SESSION **s)
{
	struct SCP_SESSION *session;
	UINT32 ipaddr;
	UINT16 cmd;
	BYTE sz;
	char buf[257];

	/* reading command */
	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != 1) /* manager login */
	{
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	session = scp_session_create();

	if (0 == session)
	{
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	scp_session_set_version(session, 1);
	scp_session_set_type(session, SCP_SESSION_TYPE_MANAGE);

	/* reading username */
	Stream_Read_UINT8(c->in_s, sz);
	buf[sz] = '\0';
	Stream_Read(c->in_s, buf, sz);

	if (0 != scp_session_set_username(session, buf))
	{
		scp_session_destroy(session);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	/* reading password */
	Stream_Read_UINT8(c->in_s, sz);
	buf[sz] = '\0';
	Stream_Read(c->in_s, buf, sz);

	if (0 != scp_session_set_password(session, buf))
	{
		scp_session_destroy(session);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	/* reading remote address */
	Stream_Read_UINT8(c->in_s, sz);

	if (sz == SCP_ADDRESS_TYPE_IPV4)
	{
		Stream_Read_UINT32_BE(c->in_s, ipaddr);
		scp_session_set_addr(session, SCP_ADDRESS_TYPE_IPV4_BIN, &ipaddr);
	}
	else if (sz == SCP_ADDRESS_TYPE_IPV6)
	{
		Stream_Read(c->in_s, buf, 16);
		scp_session_set_addr(session, SCP_ADDRESS_TYPE_IPV6_BIN, buf);
	}

	/* reading hostname */
	Stream_Read_UINT8(c->in_s, sz);
	buf[sz] = '\0';
	Stream_Read(c->in_s, buf, sz);

	if (0 != scp_session_set_hostname(session, buf))
	{
		scp_session_destroy(session);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	/* returning the struct */
	(*s) = session;

	return SCP_SERVER_STATE_START_MANAGE;
}

/* 002 */
enum SCP_SERVER_STATES_E scp_v1s_mng_allow_connection(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	Stream_Write_UINT32_BE(c->out_s, 1);
	/* packet size: 4 + 4 + 2 + 2 */
	/* version + size + cmdset + cmd */
	Stream_Write_UINT32_BE(c->out_s, 12);
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_MANAGE);
	Stream_Write_UINT16_BE(c->out_s, SCP_CMD_MNG_LOGIN_ALLOW);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, 12))
	{
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	return _scp_v1s_mng_check_response(c, s);
}

/* 003 */
enum SCP_SERVER_STATES_E scp_v1s_mng_deny_connection(struct SCP_CONNECTION *c, char *reason)
{
	int rlen;

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	/* forcing message not to exceed 64k */
	rlen = g_strlen(reason);

	if (rlen > 65535)
	{
		rlen = 65535;
	}

	Stream_Write_UINT32_BE(c->out_s, 1);
	/* packet size: 4 + 4 + 2 + 2 + 2 + strlen(reason)*/
	/* version + size + cmdset + cmd + msglen + msg */
	Stream_Write_UINT32_BE(c->out_s, rlen + 14);
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_MANAGE);
	Stream_Write_UINT16_BE(c->out_s, SCP_CMD_MNG_LOGIN_DENY);
	Stream_Write_UINT16_BE(c->out_s, rlen);
	Stream_Write(c->out_s, reason, rlen);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, rlen + 14))
	{
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	return SCP_SERVER_STATE_END;
}

/* 006 */
enum SCP_SERVER_STATES_E scp_v1s_mng_list_sessions(struct SCP_CONNECTION *c, struct SCP_SESSION *s,
		int sescnt, struct SCP_DISCONNECTED_SESSION *ds)
{
	int length;
	UINT32 version = 1;
	UINT32 size = 12;
	UINT16 cmd = SCP_CMD_MNG_LIST;
	int pktcnt;
	int idx;
	int sidx;
	int pidx;
	struct SCP_DISCONNECTED_SESSION *cds;

	/* calculating the number of packets to send */
	pktcnt = sescnt / SCP_SERVER_MAX_LIST_SIZE;

	if ((sescnt % SCP_SERVER_MAX_LIST_SIZE) != 0)
	{
		pktcnt++;
	}

	for (idx = 0; idx < pktcnt; idx++)
	{
		/* ok, we send session session list */
		Stream_Clear(c->out_s);
		Stream_SetPosition(c->out_s, 0);

		/* size: ver+size+cmdset+cmd+sescnt+continue+count */
		size = 4 + 4 + 2 + 2 + 4 + 1 + 1;

		/* header */
		c->out_s->pointer = c->out_s->buffer;
		c->out_s->pointer += 8;

		Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_MANAGE);
		Stream_Write_UINT16_BE(c->out_s, cmd);

		/* session count */
		Stream_Write_UINT32_BE(c->out_s, sescnt);

		/* setting the continue flag */
		if ((idx + 1)*SCP_SERVER_MAX_LIST_SIZE >= sescnt)
		{
			Stream_Write_UINT8(c->out_s, 0);
			/* setting session count for this packet */
			pidx = sescnt - (idx * SCP_SERVER_MAX_LIST_SIZE);
			Stream_Write_UINT8(c->out_s, pidx);
		}
		else
		{
			Stream_Write_UINT8(c->out_s, 1);
			/* setting session count for this packet */
			pidx = SCP_SERVER_MAX_LIST_SIZE;
			Stream_Write_UINT8(c->out_s, pidx);
		}

		/* adding session descriptors */
		for (sidx = 0; sidx < pidx; sidx++)
		{
			/* shortcut to the current session to send */
			cds = ds + ((idx) * SCP_SERVER_MAX_LIST_SIZE) + sidx;

			/* session data */
			Stream_Write_UINT32_BE(c->out_s, cds->SID); /* session id */
			Stream_Write_UINT8(c->out_s, cds->type);
			Stream_Write_UINT16_BE(c->out_s, cds->height);
			Stream_Write_UINT16_BE(c->out_s, cds->width);
			Stream_Write_UINT8(c->out_s, cds->bpp);
			Stream_Write_UINT8(c->out_s, cds->idle_days);
			Stream_Write_UINT8(c->out_s, cds->idle_hours);
			Stream_Write_UINT8(c->out_s, cds->idle_minutes);
			size += 13;

			Stream_Write_UINT16_BE(c->out_s, cds->conn_year);
			Stream_Write_UINT8(c->out_s, cds->conn_month);
			Stream_Write_UINT8(c->out_s, cds->conn_day);
			Stream_Write_UINT8(c->out_s, cds->conn_hour);
			Stream_Write_UINT8(c->out_s, cds->conn_minute);
			Stream_Write_UINT8(c->out_s, cds->addr_type);
			size += 7;

			if (cds->addr_type == SCP_ADDRESS_TYPE_IPV4)
			{
				Stream_Read_UINT32_BE(c->out_s, cds->ipv4addr);
				size += 4;
			}
			else if (cds->addr_type == SCP_ADDRESS_TYPE_IPV6)
			{
				Stream_Read(c->out_s, cds->ipv6addr, 16);
				size += 16;
			}
		}

		length = (int) (c->out_s->pointer - c->out_s->buffer);
		c->out_s->pointer = c->out_s->buffer;

		Stream_Write_UINT32_BE(c->out_s, version);
		Stream_Write_UINT32_BE(c->out_s, size);

		c->out_s->pointer = c->out_s->buffer + length;

		if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
		{
			log_message(LOG_LEVEL_WARNING, "[v1s_mng:%d] connection aborted: network error", __LINE__);
			return SCP_SERVER_STATE_NETWORK_ERR;
		}
	}

	return _scp_v1s_mng_check_response(c, s);
}

static enum SCP_SERVER_STATES_E _scp_v1s_mng_check_response(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
	UINT32 version;
	UINT32 size;
	UINT16 cmd;

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s_mng:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, version);

	if (version != 1)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s_mng:%d] connection aborted: version error", __LINE__);
		return SCP_SERVER_STATE_VERSION_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	/* read the rest of the packet */
	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, size - 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s_mng:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != SCP_COMMAND_SET_MANAGE)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s_mng:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd == SCP_CMD_MNG_LIST_REQ) /* request session list */
	{
		log_message(LOG_LEVEL_INFO, "[v1s_mng:%d] request session list", __LINE__);
		return SCP_SERVER_STATE_MNG_LISTREQ;
	}
	else if (cmd == SCP_CMD_MNG_ACTION) /* execute an action */
	{
		log_message(LOG_LEVEL_INFO, "[v1s_mng:%d] action request", __LINE__);
		return SCP_SERVER_STATE_MNG_ACTION;
	}

	log_message(LOG_LEVEL_WARNING, "[v1s_mng:%d] connection aborted: sequence error", __LINE__);
	return SCP_SERVER_STATE_SEQUENCE_ERR;
}

#endif
