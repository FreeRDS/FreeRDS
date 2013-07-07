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
 * @file libscp_v0.c
 * @brief libscp version 0 code
 * @author Simone Fedele
 *
 */

#include "libscp.h"

#include "os_calls.h"

extern struct log_config *s_log;

/* client API */

enum SCP_CLIENT_STATES_E scp_client_connect(struct SCP_CONNECTION *c, struct SCP_SESSION *s)
{
	int length;
	UINT32 version;
	UINT32 size;
	UINT16 sz;

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	Stream_Clear(c->out_s);
	Stream_SetPosition(c->out_s, 0);

	LOG_DBG("[v0:%d] starting connection", __LINE__);
	g_tcp_set_non_blocking(c->in_sck);
	g_tcp_set_no_delay(c->in_sck);

	c->out_s->pointer += 8;

	/* code */
	if (s->type == SCP_SESSION_TYPE_XRDP)
	{
		Stream_Write_UINT16_BE(c->out_s, 10);
	}
	else
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
		return SCP_CLIENT_STATE_INTERNAL_ERR;
	}

	sz = g_strlen(s->username);
	Stream_Write_UINT16_BE(c->out_s, sz);
	Stream_Write(c->out_s, s->username, sz);

	sz = g_strlen(s->password);
	Stream_Write_UINT16_BE(c->out_s, sz);
	Stream_Write(c->out_s, s->password, sz);
	Stream_Write_UINT16_BE(c->out_s, s->width);
	Stream_Write_UINT16_BE(c->out_s, s->height);
	Stream_Write_UINT16_BE(c->out_s, s->bpp);

	length = (int) (c->out_s->pointer - c->out_s->buffer);
	c->out_s->pointer = c->out_s->buffer;

	Stream_Write_UINT32_BE(c->out_s, 0); /* version */
	Stream_Write_UINT32_BE(c->out_s, length); /* size */

	c->out_s->pointer = c->out_s->buffer + length;

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, c->out_s->pointer - c->out_s->buffer))
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, version);

	if (0 != version)
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: version error", __LINE__);
		return SCP_CLIENT_STATE_VERSION_ERR;
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	if (size < 14)
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: packet size error", __LINE__);
		return SCP_CLIENT_STATE_SIZE_ERR;
	}

	/* getting payload */
	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, size - 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
		return SCP_CLIENT_STATE_NETWORK_ERR;
	}

	/* check code */
	Stream_Read_UINT16_BE(c->in_s, sz);

	if (3 != sz)
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: sequence error", __LINE__);
		return SCP_CLIENT_STATE_SEQUENCE_ERR;
	}

	/* message payload */
	Stream_Read_UINT16_BE(c->in_s, sz);

	if (1 != sz)
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: connection denied", __LINE__);
		return SCP_CLIENT_STATE_CONNECTION_DENIED;
	}

	Stream_Read_UINT16_BE(c->in_s, sz);
	s->display = sz;

	LOG_DBG("[v0:%d] connection terminated", __LINE__);
	return SCP_CLIENT_STATE_END;
}

/* server API */

enum SCP_SERVER_STATES_E scp_server_accept(struct SCP_CONNECTION *c, struct SCP_SESSION **s, int skipVchk)
{
	UINT32 version = 0;
	UINT32 size;
	struct SCP_SESSION *session = 0;
	UINT16 sz;
	UINT32 code = 0;
	char buf[257];

	if (!skipVchk)
	{
		LOG_DBG("[v0:%d] starting connection", __LINE__);

		if (0 == scp_tcp_force_recv(c->in_sck, c->in_s->buffer, 8))
		{
			c->in_s->length = 8;
			Stream_Read_UINT32_BE(c->in_s, version);

			if (version != 0)
			{
				log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: version error", __LINE__);
				return SCP_SERVER_STATE_VERSION_ERR;
			}
		}
		else
		{
			log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
			return SCP_SERVER_STATE_NETWORK_ERR;
		}
	}

	Stream_Read_UINT32_BE(c->in_s, size);

	Stream_Clear(c->in_s);
	Stream_SetPosition(c->in_s, 0);

	if (0 != scp_tcp_force_recv(c->in_sck, c->in_s->buffer, size - 8))
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	c->in_s->length = size - 8;

	Stream_Read_UINT16_BE(c->in_s, code);

	if (code == 0 || code == 10)
	{
		session = scp_session_create();

		if (0 == session)
		{
			log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
			return SCP_SERVER_STATE_INTERNAL_ERR;
		}

		scp_session_set_version(session, version);

		scp_session_set_type(session, SCP_SESSION_TYPE_XRDP);

		/* reading username */
		Stream_Read_UINT16_BE(c->in_s, sz);
		buf[sz] = '\0';
		Stream_Read(c->in_s, buf, sz);

		if (0 != scp_session_set_username(session, buf))
		{
			scp_session_destroy(session);
			log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: error setting username", __LINE__);
			return SCP_SERVER_STATE_INTERNAL_ERR;
		}

		/* reading password */
		Stream_Read_UINT16_BE(c->in_s, sz);
		buf[sz] = '\0';
		Stream_Read(c->in_s, buf, sz);

		if (0 != scp_session_set_password(session, buf))
		{
			scp_session_destroy(session);
			log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: error setting password", __LINE__);
			return SCP_SERVER_STATE_INTERNAL_ERR;
		}

		/* width */
		Stream_Read_UINT16_BE(c->in_s, sz);
		scp_session_set_width(session, sz);
		/* height */
		Stream_Read_UINT16_BE(c->in_s, sz);
		scp_session_set_height(session, sz);
		/* bpp */
		Stream_Read_UINT16_BE(c->in_s, sz);
		scp_session_set_bpp(session, (BYTE)sz);

		if (Stream_GetRemainingLength(c->in_s) >= 2)
		{
			/* reading domain */
			Stream_Read_UINT16_BE(c->in_s, sz);

			if (sz > 0)
			{
				Stream_Read(c->in_s, buf, sz);
				buf[sz] = '\0';
				scp_session_set_domain(session, buf);
			}
		}

		if (Stream_GetRemainingLength(c->in_s) >= 2)
		{
			/* reading program */
			Stream_Read_UINT16_BE(c->in_s, sz);

			if (sz > 0)
			{
				Stream_Read(c->in_s, buf, sz);
				buf[sz] = '\0';
				scp_session_set_program(session, buf);
			}
		}

		if (Stream_GetRemainingLength(c->in_s) >= 2)
		{
			/* reading directory */
			Stream_Read_UINT16_BE(c->in_s, sz);

			if (sz > 0)
			{
				Stream_Read(c->in_s, buf, sz);
				buf[sz] = '\0';
				scp_session_set_directory(session, buf);
			}
		}

		if (Stream_GetRemainingLength(c->in_s) >= 2)
		{
			/* reading client IP address */
			Stream_Read_UINT16_BE(c->in_s, sz);

			if (sz > 0)
			{
				Stream_Read(c->in_s, buf, sz);
				buf[sz] = '\0';
				scp_session_set_client_ip(session, buf);
			}
		}
	}
	else if (code == SCP_GW_AUTHENTICATION)
	{
		/* g_writeln("Command is SCP_GW_AUTHENTICATION"); */
		session = scp_session_create();

		if (0 == session)
		{
			/* until syslog merge log_message(s_log, LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error",      __LINE__);*/
			return SCP_SERVER_STATE_INTERNAL_ERR;
		}

		scp_session_set_version(session, version);
		scp_session_set_type(session, SCP_GW_AUTHENTICATION);
		/* reading username */
		Stream_Read_UINT16_BE(c->in_s, sz);
		buf[sz] = '\0';
		Stream_Read(c->in_s, buf, sz);

		/* g_writeln("Received user name: %s",buf); */
		if (0 != scp_session_set_username(session, buf))
		{
			scp_session_destroy(session);
			/* until syslog merge log_message(s_log, LOG_LEVEL_WARNING, "[v0:%d] connection aborted: error setting        username", __LINE__);*/
			return SCP_SERVER_STATE_INTERNAL_ERR;
		}

		/* reading password */
		Stream_Read_UINT16_BE(c->in_s, sz);
		buf[sz] = '\0';
		Stream_Read(c->in_s, buf, sz);

		/* g_writeln("Received password: %s",buf); */
		if (0 != scp_session_set_password(session, buf))
		{
			scp_session_destroy(session);
			/* until syslog merge log_message(s_log, LOG_LEVEL_WARNING, "[v0:%d] connection aborted: error setting password", __LINE__); */
			return SCP_SERVER_STATE_INTERNAL_ERR;
		}
	}
	else
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: sequence error", __LINE__);
		return SCP_SERVER_STATE_SEQUENCE_ERR;
	}

	(*s) = session;
	return SCP_SERVER_STATE_OK;
}

enum SCP_SERVER_STATES_E scp_server_allow_connection(struct SCP_CONNECTION *c, SCP_DISPLAY d)
{
	int length;

	Stream_Write_UINT32_BE(c->out_s, 0);  /* version */
	Stream_Write_UINT32_BE(c->out_s, 14); /* size */
	Stream_Write_UINT16_BE(c->out_s, 3);  /* cmd */
	Stream_Write_UINT16_BE(c->out_s, 1);  /* data */
	Stream_Write_UINT16_BE(c->out_s, d);  /* data */

	length = (int) (c->out_s->pointer - c->out_s->buffer);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, length))
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	LOG_DBG("[v0:%d] connection terminated (allowed)", __LINE__);
	return SCP_SERVER_STATE_OK;
}

enum SCP_SERVER_STATES_E scp_server_deny_connection(struct SCP_CONNECTION *c)
{
	int length;

	Stream_Write_UINT32_BE(c->out_s, 0);  /* version */
	Stream_Write_UINT32_BE(c->out_s, 14); /* size */
	Stream_Write_UINT16_BE(c->out_s, 3);  /* cmd */
	Stream_Write_UINT16_BE(c->out_s, 0);  /* data = 0 - means NOT ok*/
	Stream_Write_UINT16_BE(c->out_s, 0);  /* reserved for display number*/

	length = (int) (c->out_s->pointer - c->out_s->buffer);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, length))
	{
		log_message(LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__);
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	LOG_DBG("[v0:%d] connection terminated (denied)", __LINE__);
	return SCP_SERVER_STATE_OK;
}

enum SCP_SERVER_STATES_E scp_server_replyauthentication(struct SCP_CONNECTION *c, unsigned short int value)
{
	int length;

	Stream_Write_UINT32_BE(c->out_s, 0);  /* version */
	Stream_Write_UINT32_BE(c->out_s, 14); /* size */
	/* cmd SCP_GW_AUTHENTICATION means authentication reply */
	Stream_Write_UINT16_BE(c->out_s, SCP_GW_AUTHENTICATION);
	Stream_Write_UINT16_BE(c->out_s, value);  /* reply code  */
	Stream_Write_UINT16_BE(c->out_s, 0);  /* dummy data */

	length = (int) (c->out_s->pointer - c->out_s->buffer);

	if (0 != scp_tcp_force_send(c->in_sck, c->out_s->buffer, length))
	{
		/* until syslog merge log_message(s_log, LOG_LEVEL_WARNING, "[v0:%d] connection aborted: network error", __LINE__); */
		return SCP_SERVER_STATE_NETWORK_ERR;
	}

	/* until syslog merge LOG_DBG(s_log, "[v0:%d] connection terminated (scp_server_deny_authentication)", __LINE__);*/
	return SCP_SERVER_STATE_OK;
}
