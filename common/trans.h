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
 * generic transport
 */

#if !defined(TRANS_H)
#define TRANS_H

#include <winpr/crt.h>
#include <winpr/stream.h>

#define TRANS_MODE_TCP 1
#define TRANS_MODE_UNIX 2

#define TRANS_TYPE_LISTENER 1
#define TRANS_TYPE_SERVER 2
#define TRANS_TYPE_CLIENT 3

#define TRANS_STATUS_DOWN 0
#define TRANS_STATUS_UP 1

struct trans; /* forward declaration */

typedef int (*ttrans_data_in)(struct trans* self);
typedef int (*ttrans_conn_in)(struct trans* self, struct trans* new_self);

struct trans
{
	LONG_PTR sck; /* socket handle */
	int mode; /* 1 tcp, 2 unix socket */
	int status;
	int type1; /* 1 listener 2 server 3 client */
	ttrans_data_in trans_data_in;
	ttrans_conn_in trans_conn_in;
	void* callback_data;
	int header_size;
	wStream* in_s;
	wStream* out_s;
	char* listen_filename;
};

struct trans* trans_create(int mode, int in_size, int out_size);
void trans_delete(struct trans* self);
int trans_get_wait_objs(struct trans* self, LONG_PTR* objs, int* count);
int trans_check_wait_objs(struct trans* self);
int trans_force_read_s(struct trans* self, wStream* in_s, int size);
int trans_force_write_s(struct trans* self, wStream* out_s);
int trans_force_read(struct trans* self, int size);
int trans_force_write(struct trans* self);
int trans_connect(struct trans* self, const char* server, const char* port, int timeout);
int trans_listen_address(struct trans* self, char* port, const char* address);
int trans_listen(struct trans* self, char* port);
wStream* trans_get_in_s(struct trans* self);
wStream* trans_get_out_s(struct trans* self, int size);

#endif
