/**
 * pbRPC - a simple, protocol buffer based RCP mechanism
 * Utility functions
 *
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bmiklautz@thinstuff.at>
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
#ifndef _PBRPC_UTILS_H
#define _PBRPC_UTILS_H
#include "pbrpc.h"
#include "pbRPC.pb-c.h"

DWORD pbrpc_getTag(pbRPCContext *context);
Freerds__Pbrpc__RPCBase *pbrpc_message_new();
void pbrpc_message_free(Freerds__Pbrpc__RPCBase *msg, BOOL freePayload);
void pbrpc_prepare_request(pbRPCContext *context, Freerds__Pbrpc__RPCBase *msg);
void pbrpc_prepare_response(Freerds__Pbrpc__RPCBase *msg, UINT32 tag);
void pbrpc_prepare_error(Freerds__Pbrpc__RPCBase *msg, UINT32 tag, char *error);
pbRPCPayload *pbrpc_payload_new();
void pbrpc_free_payload(pbRPCPayload *response);

#endif // _PBRPC_UTILS_H
