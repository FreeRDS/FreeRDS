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
 *
 * main include file
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef RDS_H
#define RDS_H

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>

#include <freerds/freerds.h>

#include <pixman.h>

typedef struct xrdp_listener xrdpListener;

#include "xrdp_core.h"

int g_is_term(void);
void g_set_term(int in_val);
HANDLE g_get_term_event(void);

rdsSession* xrdp_process_create(freerdp_peer* client);
void xrdp_process_delete(rdsSession* self);
HANDLE xrdp_process_get_term_event(rdsSession* self);
void* xrdp_process_main_thread(void* arg);

xrdpListener* xrdp_listen_create(void);
void xrdp_listen_delete(xrdpListener* self);
int xrdp_listen_main_loop(xrdpListener* self);

rdsModule* xrdp_module_new(rdsSession* session);
void xrdp_module_free(rdsModule* mod);

long xrdp_authenticate(char* username, char* password, int* errorcode);

void* xrdp_client_thread(void* arg);

int freerds_client_inbound_module_init(rdsModule* mod);
int xrdp_message_server_module_init(rdsModule* mod);

int xrdp_message_server_queue_pack(rdsModule* mod);
int xrdp_message_server_queue_process_pending_messages(rdsModule* mod);
int xrdp_message_server_module_init(rdsModule* mod);

#endif /* RDS_H */
