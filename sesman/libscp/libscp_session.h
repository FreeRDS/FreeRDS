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
 * @file libscp_session.h
 * @brief SCP_SESSION handling code
 * @author Simone Fedele
 *
 */

#ifndef LIBSCP_SESSION_H
#define LIBSCP_SESSION_H

#include "libscp.h"

/**
 *
 * @brief creates a new connection
 * @param sck the connection socket
 *
 * @return a SCP_SESSION* object on success, NULL otherwise
 *
 */
SCP_SESSION* scp_session_create();

int scp_session_set_type(SCP_SESSION* s, BYTE type);
int scp_session_set_version(SCP_SESSION* s, UINT32 version);
int scp_session_set_height(SCP_SESSION* s, UINT16 h);
int scp_session_set_width(SCP_SESSION* s, UINT16 w);
int scp_session_set_bpp(SCP_SESSION* s, BYTE bpp);
int scp_session_set_rsr(SCP_SESSION* s, BYTE rsr);
int scp_session_set_locale(SCP_SESSION* s, char* str);
int scp_session_set_username(SCP_SESSION* s, char* str);
int scp_session_set_password(SCP_SESSION* s, char* str);
int scp_session_set_domain(SCP_SESSION* s, char* str);
int scp_session_set_program(SCP_SESSION* s, char* str);
int scp_session_set_directory(SCP_SESSION* s, char* str);
int scp_session_set_client_ip(SCP_SESSION* s, char* str);
int scp_session_set_hostname(SCP_SESSION* s, char* str);
int scp_session_set_addr(SCP_SESSION* s, int type, void* addr);
int scp_session_set_display(SCP_SESSION* s, SCP_DISPLAY display);
int scp_session_set_errstr(SCP_SESSION* s, char* str);

/**
 *
 * @brief destroys a session object
 * @param s the object to be destroyed
 *
 */
void scp_session_destroy(SCP_SESSION* s);

#endif
