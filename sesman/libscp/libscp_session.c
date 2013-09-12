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
 * @file libscp_session.c
 * @brief SCP_SESSION handling code
 * @author Simone Fedele
 *
 */

#include "libscp_session.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <winpr/crt.h>

SCP_SESSION* scp_session_create()
{
	SCP_SESSION* s;

	s = (SCP_SESSION*) malloc(sizeof(SCP_SESSION));

	if (s)
	{
		ZeroMemory(s, sizeof(SCP_SESSION));
	}

	return s;
}

int scp_session_set_type(SCP_SESSION* s, BYTE type)
{
	switch (type)
	{
		case SCP_SESSION_TYPE_XRDP:
			s->type = SCP_SESSION_TYPE_XRDP;
			break;

		default:
			return 1;
	}

	return 0;
}

int scp_session_set_version(SCP_SESSION* s, UINT32 version)
{
	switch (version)
	{
		case 0:
			s->version = 0;
			break;

		default:
			return 1;
	}

	return 0;
}

int scp_session_set_height(SCP_SESSION* s, UINT16 h)
{
	s->height = h;
	return 0;
}

int scp_session_set_width(SCP_SESSION* s, UINT16 w)
{
	s->width = w;
	return 0;
}

int scp_session_set_bpp(SCP_SESSION* s, BYTE bpp)
{
	switch (bpp)
	{
		case 8:
		case 15:
		case 16:
		case 24:
			s->bpp = bpp;
		default:
			return 1;
	}

	return 0;
}

int scp_session_set_rsr(SCP_SESSION* s, BYTE rsr)
{
	if (s->rsr)
	{
		s->rsr = 1;
	}
	else
	{
		s->rsr = 0;
	}

	return 0;
}

int scp_session_set_locale(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_locale: null locale", __LINE__);
		s->locale[0] = '\0';
		return 1;
	}

	g_strncpy(s->locale, str, 17);
	s->locale[17] = '\0';
	return 0;
}

int scp_session_set_username(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_username: null username", __LINE__);
		return 1;
	}

	if (s->username)
	{
		free(s->username);
	}

	s->username = g_strdup(str);

	if (!s->username)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_username: strdup error", __LINE__);
		return 1;
	}

	return 0;
}

int scp_session_set_password(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_password: null password", __LINE__);
		return 1;
	}

	if (s->password)
	{
		free(s->password);
	}

	s->password = g_strdup(str);

	if (!s->password)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_password: strdup error", __LINE__);
		return 1;
	}

	return 0;
}

int scp_session_set_domain(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_domain: null domain", __LINE__);
		return 1;
	}

	if (s->domain)
	{
		free(s->domain);
	}

	s->domain = g_strdup(str);

	if (!s->domain)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_domain: strdup error", __LINE__);
		return 1;
	}

	return 0;
}

int scp_session_set_program(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_program: null program", __LINE__);
		return 1;
	}

	if (s->program)
	{
		free(s->program);
	}

	s->program = g_strdup(str);

	if (!s->program)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_program: strdup error", __LINE__);
		return 1;
	}

	return 0;
}

int scp_session_set_directory(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_directory: null directory", __LINE__);
		return 1;
	}

	if (s->directory)
	{
		free(s->directory);
	}

	s->directory = g_strdup(str);

	if (!s->directory)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_directory: strdup error", __LINE__);
		return 1;
	}

	return 0;
}

int scp_session_set_client_ip(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_client_ip: null ip", __LINE__);
		return 1;
	}

	if (s->client_ip)
	{
		free(s->client_ip);
	}

	s->client_ip = g_strdup(str);

	if (!s->client_ip)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_client_ip: strdup error", __LINE__);
		return 1;
	}

	return 0;
}

int scp_session_set_hostname(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_hostname: null hostname", __LINE__);
		return 1;
	}

	if (s->hostname)
	{
		free(s->hostname);
	}

	s->hostname = g_strdup(str);

	if (!s->hostname)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_hostname: strdup error", __LINE__);
		return 1;
	}

	return 0;
}

int scp_session_set_errstr(SCP_SESSION* s, char *str)
{
	if (!str)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_errstr: null string", __LINE__);
		return 1;
	}

	if (s->errstr)
	{
		free(s->errstr);
	}

	s->errstr = g_strdup(str);

	if (!s->errstr)
	{
		log_message(LOG_LEVEL_WARNING, "[session:%d] set_errstr: strdup error", __LINE__);
		return 1;
	}

	return 0;
}

int scp_session_set_display(SCP_SESSION* s, SCP_DISPLAY display)
{
	s->display = display;
	return 0;
}

int scp_session_set_addr(SCP_SESSION* s, int type, void *addr)
{
	struct in_addr ip4;
#ifdef IN6ADDR_ANY_INIT
	struct in6_addr ip6;
#endif
	int ret;

	switch (type)
	{
		case SCP_ADDRESS_TYPE_IPV4:
			/* convert from char to 32bit*/
			ret = inet_pton(AF_INET, addr, &ip4);

			if (!ret)
			{
				log_message(LOG_LEVEL_WARNING, "[session:%d] set_addr: invalid address", __LINE__);
				inet_pton(AF_INET, "127.0.0.1", &ip4);
				g_memcpy(&(s->ipv4addr), &(ip4.s_addr), 4);
				return 1;
			}

			g_memcpy(&(s->ipv4addr), &(ip4.s_addr), 4);
			break;
		case SCP_ADDRESS_TYPE_IPV4_BIN:
			g_memcpy(&(s->ipv4addr), addr, 4);
			break;
#ifdef IN6ADDR_ANY_INIT
		case SCP_ADDRESS_TYPE_IPV6:
			/* convert from char to 128bit*/
			ret = inet_pton(AF_INET6, addr, &ip6);

			if (!ret)
			{
				log_message(LOG_LEVEL_WARNING, "[session:%d] set_addr: invalid address", __LINE__);
				inet_pton(AF_INET, "::1", &ip6);
				g_memcpy(s->ipv6addr, &(ip6.s6_addr), 16);
				return 1;
			}

			g_memcpy(s->ipv6addr, &(ip6.s6_addr), 16);
			break;
		case SCP_ADDRESS_TYPE_IPV6_BIN:
			g_memcpy(s->ipv6addr, addr, 16);
			break;
#endif
		default:
			return 1;
	}

	return 0;
}

void scp_session_destroy(SCP_SESSION* s)
{
	if (s)
	{
		free(s->username);
		free(s->password);
		free(s->hostname);
		free(s->domain);
		free(s->program);
		free(s->directory);
		free(s->client_ip);
		free(s->errstr);
		free(s);
	}
}
