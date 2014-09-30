/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef FREERDS_H
#define FREERDS_H

#include <winpr/crt.h>
#include <winpr/synch.h>
#include <winpr/stream.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>
#include <freerdp/channels/channels.h>

#include <freerds/backend.h>

typedef struct xrdp_listener rdsListener;

#include "core.h"

int g_is_term(void);
void g_set_term(int in_val);
HANDLE g_get_term_event(void);

rdsConnection* freerds_connection_create(freerdp_peer* client);
void freerds_connection_delete(rdsConnection* self);
HANDLE freerds_connection_get_term_event(rdsConnection* self);
void* freerds_connection_main_thread(void* arg);

rdsListener* freerds_listener_create(void);
void freerds_listener_delete(rdsListener* self);
int freerds_listener_main_loop(rdsListener* self);

rdsBackendConnector* freerds_connector_new(rdsConnection* connection);
void freerds_connector_free(rdsBackendConnector* connector);
BOOL freerds_connector_connect(rdsBackendConnector* connector);

int freerds_init_client(HANDLE hClientPipe, rdpSettings* settings, wStream* s);

void* freerds_client_thread(void* arg);
int freerds_client_get_event_handles(rdsBackend* backend, HANDLE* events, DWORD* nCount);
int freerds_client_check_event_handles(rdsBackend* backend);

int freerds_client_inbound_connector_init(rdsBackendConnector* connector);
int freerds_message_server_connector_init(rdsBackendConnector* connector);

int freerds_message_server_queue_pack(rdsBackendConnector* connector);
int freerds_message_server_queue_process_pending_messages(rdsBackendConnector* backend);

#endif /* FREERDS_H */
