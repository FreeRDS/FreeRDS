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
 * @file libscp_v1s.c
 * @brief libscp version 1 server api code
 * @author Simone Fedele
 *
 */

#ifndef LIBSCP_V1S_C
#define LIBSCP_V1S_C

#include "libscp_v1s.h"

/* server API */
enum SCP_SERVER_STATES_E scp_v1s_accept(struct SCP_CONNECTION *c, struct SCP_SESSION **s, int skipVchk)
{
	struct SCP_SESSION *session;
	tui32 version;
	tui32 size;
	tui16 cmdset;
	tui16 cmd;
	tui8 sz;
	char buf[257];

	if (!skipVchk)
	{
		if (0 == scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
		{
			Stream_Read_UINT32_BE(c->in_s, version);

			if (version != 1)
			{
				log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: version error", __LINE__);
				return SCP_SERVER_STATE_VERSION_ERR;
			}
		}
		else
		{
			log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
			return SCP_SERVER_STATE_NETWORK_ERR;
		}
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	if (size < 12)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: size error", __LINE__);
		return SCP_SERVER_STATE_SIZE_ERR;
	}

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, (size - 8)))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	/* reading command set */
	Stream_Read_UINT16_BE(c->in_s, cmdset);

	/* if we are starting a management session */
	if (cmdset == SCP_COMMAND_SET_MANAGE)
	{
		log_message(LOG_LEVEL_DEBUG, "[v1s:%d] requested management connection", __LINE__);
		/* should return SCP_SERVER_STATE_START_MANAGE */
		return scp_v1s_mng_accept(c, s);
	}

	/* if we started with resource sharing... */
	if (cmdset == SCP_COMMAND_SET_RSR)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	/* reading command */
	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != 1)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	session = scp_session_create();

	if (0 == session)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: internal error (malloc returned NULL)", __LINE__);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	scp_session_set_version(session, 1);

	Stream_Read_UINT8(c->in_s, sz);

	if ((sz != SCP_SESSION_TYPE_XVNC) && (sz != SCP_SESSION_TYPE_XRDP))
	{
		scp_session_destroy(session);
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: unknown session type", __LINE__);
		return SCP_SERVER_STATE_SESSION_TYPE_ERR;
	}

	scp_session_set_type(session, sz);

	Stream_Read_UINT16_BE(c->in_s, cmd);
	scp_session_set_height(session, cmd);
	Stream_Read_UINT16_BE(c->in_s, cmd);
	scp_session_set_height(session, cmd);
	Stream_Read_UINT8(c->in_s, sz);
	scp_session_set_bpp(session, sz);
	Stream_Read_UINT8(c->in_s, sz);
	scp_session_set_rsr(session, sz);
	Stream_Read(c->in_s, buf, 17);
	buf[17] = '\0';
	scp_session_set_locale(session, buf);

	Stream_Read_UINT8(c->in_s, sz);

	if (sz == SCP_ADDRESS_TYPE_IPV4)
	{
		Stream_Read_UINT32_BE(c->in_s, size);
		scp_session_set_addr(session, SCP_ADDRESS_TYPE_IPV4_BIN, &size);
	}
	else if (sz == SCP_ADDRESS_TYPE_IPV6)
	{
		Stream_Read(c->in_s, buf, 16);
		scp_session_set_addr(session, SCP_ADDRESS_TYPE_IPV6_BIN, buf);
	}

	buf[256] = '\0';
	/* reading hostname */
	Stream_Read_UINT8(c->in_s, sz);
	buf[sz] = '\0';
	Stream_Read(c->in_s, buf, sz);

	if (0 != scp_session_set_hostname(session, buf))
	{
		scp_session_destroy(session);
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: internal error", __LINE__);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	/* reading username */
	Stream_Read_UINT8(c->in_s, sz);
	buf[sz] = '\0';
	Stream_Read(c->in_s, buf, sz);

	if (0 != scp_session_set_username(session, buf))
	{
		scp_session_destroy(session);
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: internal error", __LINE__);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	/* reading password */
	Stream_Read_UINT8(c->in_s, sz);
	buf[sz] = '\0';
	Stream_Read(c->in_s, buf, sz);

	if (0 != scp_session_set_password(session, buf))
	{
		scp_session_destroy(session);
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: internal error", __LINE__);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	/* returning the struct */
	(*s) = session;

	return SCP_SERVER_STATE_OK;
}

enum SCP_SERVER_STATES_E scp_v1s_deny_connection(struct SCP_CONNECTION *c, char *reason)
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
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT);
	Stream_Write_UINT16_BE(c->out_s, 2);
	Stream_Write_UINT16_BE(c->out_s, rlen);
	Stream_Write(c->out_s, reason, rlen);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, rlen + 14))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	return SCP_SERVER_STATE_END;
}

enum SCP_SERVER_STATES_E scp_v1s_request_password(struct SCP_CONNECTION *c, struct SCP_SESSION *s, char *reason)
{
	tui8 sz;
	tui32 version;
	tui32 size;
	tui16 cmdset;
	tui16 cmd;
	int rlen;
	char buf[257];

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	/* forcing message not to exceed 64k */
	rlen = g_strlen(reason);

	if (rlen > 65535)
	{
		rlen = 65535;
	}

	/* send password request */
	version = 1;
	cmd = 3;

	Stream_Write_UINT32_BE(c->out_s, version);                 /* version */
	Stream_Write_UINT32_BE(c->out_s, 14 + rlen);               /* size    */
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT); /* cmdset  */
	Stream_Write_UINT16_BE(c->out_s, cmd);                     /* cmd     */

	Stream_Write_UINT16_BE(c->out_s, rlen);
	Stream_Write(c->out_s, reason, rlen);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, 14 + rlen))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	/* receive password & username */
	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, version);

	if (version != 1)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: version error", __LINE__);
		return SCP_SERVER_STATE_VERSION_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	if (size < 12)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: size error", __LINE__);
		return SCP_SERVER_STATE_SIZE_ERR;
	}

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, (size - 8)))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmdset);

	if (cmdset != SCP_COMMAND_SET_DEFAULT)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != 4)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	buf[256] = '\0';
	/* reading username */
	Stream_Read_UINT8(c->in_s, sz);
	buf[sz] = '\0';
	Stream_Read(c->in_s, buf, sz);

	if (0 != scp_session_set_username(s, buf))
	{
		scp_session_destroy(s);
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: internal error", __LINE__);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	/* reading password */
	Stream_Read_UINT8(c->in_s, sz);
	buf[sz] = '\0';
	Stream_Read(c->in_s, buf, sz);

	if (0 != scp_session_set_password(s, buf))
	{
		scp_session_destroy(s);
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: internal error", __LINE__);
		return SCP_SERVER_STATE_INTERNAL_ERR;
	}

	return SCP_SERVER_STATE_OK;
}

/* 020 */
enum SCP_SERVER_STATES_E scp_v1s_request_pwd_change(struct SCP_CONNECTION *c, char *reason, char *npw)
{
	return SCP_SERVER_STATE_INTERNAL_ERR;
}

/* 023 */
enum SCP_SERVER_STATES_E scp_v1s_pwd_change_error(struct SCP_CONNECTION *c, char *error, int retry, char *npw)
{
	return SCP_SERVER_STATE_INTERNAL_ERR;
}

/* 030 */
enum SCP_SERVER_STATES_E scp_v1s_connect_new_session(struct SCP_CONNECTION *c, SCP_DISPLAY d)
{
	/* send password request */
	tui32 version = 1;
	tui32 size = 14;
	tui16 cmd = 30;

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	Stream_Write_UINT32_BE(c->out_s, version);                 /* version */
	Stream_Write_UINT32_BE(c->out_s, size);                    /* size    */
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT); /* cmdset  */
	Stream_Write_UINT16_BE(c->out_s, cmd);                     /* cmd     */

	Stream_Write_UINT16_BE(c->out_s, d);                       /* display */

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, 14))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	return SCP_SERVER_STATE_OK;
}

/* 032 */
enum SCP_SERVER_STATES_E scp_v1s_connection_error(struct SCP_CONNECTION *c, char *error)
{
	tui16 len;

	len = g_strlen(error);

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	Stream_Write_UINT32_BE(c->out_s, 1);
	/* packet size: 4 + 4 + 2 + 2 + len */
	/* version + size + cmdset + cmd */
	Stream_Write_UINT32_BE(c->out_s, (12 + len));
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT);
	Stream_Write_UINT16_BE(c->out_s, SCP_CMD_CONN_ERROR);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, (12 + len)))
	{
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	return SCP_SERVER_STATE_END;
}

/* 040 */
enum SCP_SERVER_STATES_E scp_v1s_list_sessions(struct SCP_CONNECTION *c, int sescnt, struct SCP_DISCONNECTED_SESSION *ds, SCP_SID *sid)
{
	int length;
	tui32 version = 1;
	tui32 size = 12;
	tui16 cmd = 40;
	int pktcnt;
	int idx;
	int sidx;
	int pidx;
	struct SCP_DISCONNECTED_SESSION *cds;

	/* first we send a notice that we have some disconnected sessions */
	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	Stream_Write_UINT32_BE(c->out_s, version);                 /* version */
	Stream_Write_UINT32_BE(c->out_s, size);                    /* size    */
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT); /* cmdset  */
	Stream_Write_UINT16_BE(c->out_s, cmd);                     /* cmd     */

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	/* then we wait for client ack */
	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, version);

	if (version != 1)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: version error", __LINE__);
		return SCP_SERVER_STATE_VERSION_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	if (size < 12)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: size error", __LINE__);
		return SCP_SERVER_STATE_SIZE_ERR;
	}

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, (size - 8)))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != SCP_COMMAND_SET_DEFAULT)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != 41)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

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
		cmd = 42;

		c->out_s->pointer = c->out_s->buffer;
		c->out_s->pointer += 8;

		Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT);
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
			log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
			return SCP_SERVER_STATE_NETWORK_ERR;
		}
	}

	/* we get the response */
	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, (8)))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, version);

	if (version != 1)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: version error", __LINE__);
		return SCP_SERVER_STATE_VERSION_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	if (size < 12)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: size error", __LINE__);
		return SCP_SERVER_STATE_SIZE_ERR;
	}

	/* rest of the packet */
	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, (size - 8)))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd != SCP_COMMAND_SET_DEFAULT)
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	Stream_Read_UINT16_BE(c->in_s, cmd);

	if (cmd == 43)
	{
		/* select session */
		Stream_Read_UINT32_BE(c->in_s, (*sid));

		/* checking sid value */
		for (idx = 0; idx < sescnt; idx++)
		{
			/* the sid is valid */
			if (ds[idx].SID == (*sid))
			{
				/* ok, session selected */
				return SCP_SERVER_STATE_OK;
			}
		}

		/* if we got here, the requested sid wasn't one from the list we sent */
		/* we should kill the connection                                      */
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: internal error (no such session in list)", __LINE__);
		return SCP_CLIENT_STATE_INTERNAL_ERR;
	}
	else if (cmd == 44)
	{
		/* cancel connection */
		return SCP_SERVER_STATE_SELECTION_CANCEL;
	}
	else
	{
		/* wrong response */
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	return SCP_SERVER_STATE_OK;
}

/* 046 was: 031 struct SCP_DISCONNECTED_SESSION* ds, */
enum SCP_SERVER_STATES_E scp_v1s_reconnect_session(struct SCP_CONNECTION *c, SCP_DISPLAY d)
{
	tui32 version = 1;
	tui32 size = 14;
	tui16 cmd = 46;

	/* ok, we send session data and display */
	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	/* header */
	Stream_Write_UINT32_BE(c->out_s, version);
	Stream_Write_UINT32_BE(c->out_s, size);
	Stream_Write_UINT16_BE(c->out_s, SCP_COMMAND_SET_DEFAULT);
	Stream_Write_UINT16_BE(c->out_s, cmd);

	/* session data */
	Stream_Write_UINT16_BE(c->out_s, d); /* session display */
	/* these last three are not really needed... */

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, size))
	{
		log_message(LOG_LEVEL_WARNING, "[v1s:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	return SCP_SERVER_STATE_OK;
}

#endif
