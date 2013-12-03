/**
 * pbRPC - a simple, protocol buffer based RCP mechanism
 * Transport protocol abstraction
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bernhard.miklautz@thincast.com>
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

#ifndef _PBRPC_TRANSPORT_H
#define _PBRPC_TRANSPORT_H

#include <winpr/wtypes.h>

typedef struct pbrpc_transport_context pbRPCTransportContext;

typedef int (*pTransport_open)(pbRPCTransportContext* context, int timeout);
typedef int (*pTransport_close)(pbRPCTransportContext* context);
typedef int (*pTransport_write)(pbRPCTransportContext* context, char* data, unsigned int datalen);
typedef int (*pTransport_read)(pbRPCTransportContext* context, char* data, unsigned int datalen);
typedef HANDLE (*pTransport_get_fds)(pbRPCTransportContext* context);

struct pbrpc_transport_context
{
	pTransport_open open;
	pTransport_close close;
	pTransport_read read;
	pTransport_write write;
	pTransport_get_fds get_fds;
};

#endif // _PBRPC_TRANSPORT_H
