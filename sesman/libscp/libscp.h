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
 * @file libscp.h
 * @brief libscp main header
 * @author Simone Fedele
 *
 */

#ifndef LIBSCP_H
#define LIBSCP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libscp_types.h"

#include "libscp_connection.h"
#include "libscp_session.h"
#include "libscp_init.h"
#include "libscp_tcp.h"
#include "libscp_lock.h"

/* client API */
/**
 *
 * @brief connects to sesman using scp v0
 * @param c connection descriptor
 * @param s session descriptor
 * @param d display
 *
 */
enum SCP_CLIENT_STATES_E scp_client_connect(struct SCP_CONNECTION* c, struct SCP_SESSION* s);

/* server API */
/**
 *
 * @brief processes the stream using scp version 0
 * @param c connection descriptor
 * @param s session descriptor
 * @param skipVchk if set to !0 skips the version control (to be used after
 *                 scp_vXs_accept() )
 *
 */
enum SCP_SERVER_STATES_E scp_server_accept(struct SCP_CONNECTION* c, struct SCP_SESSION** s, int skipVchk);

/**
 *
 * @brief allows the connection to TS, returning the display port
 * @param c connection descriptor
 *
 */
enum SCP_SERVER_STATES_E scp_server_allow_connection(struct SCP_CONNECTION* c, SCP_DISPLAY d);

/**
 *
 * @brief denies the connection to TS
 * @param c connection descriptor
 *
 */
enum SCP_SERVER_STATES_E scp_server_deny_connection(struct SCP_CONNECTION* c);

/**
 * @brief send reply to an authentication request
 * @param c connection descriptor
 * @param value the reply code 0 means ok
 * @return
 */
enum SCP_SERVER_STATES_E scp_server_replyauthentication(struct SCP_CONNECTION* c, unsigned short int value);

#endif
