/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#ifndef FREERDS_RPC_INTERNAL_H
#define FREERDS_RPC_INTERNAL_H

#include "msgpack.h"

#include <winpr/crt.h>
#include <winpr/stream.h>

#include <freerds/rpc.h>

typedef wStream* (*pRdsRpcPack)(void* data, wStream* s);
typedef BOOL (*pRdsRpcUnpack)(void* data, const BYTE* buffer, UINT32 size);
typedef void (*pRdsRpcFree)(void* data);

struct _RDS_RPC_PACK_FUNC
{
	UINT32 msgType;
	pRdsRpcPack Pack;
	pRdsRpcUnpack Unpack;
	pRdsRpcFree Free;
};
typedef struct _RDS_RPC_PACK_FUNC RDS_RPC_PACK_FUNC;

/* msgpack helpers */

#define STREAM_PACK_PREPARE(_s) \
	if (!_s) _s = Stream_New(NULL, 512); \
	if (!_s) return NULL

#define STREAM_PACK_FINALIZE(_s) \
	Stream_SealLength(_s); \
	Stream_SetPosition(_s, 0)

BOOL msgpack_unpack_array(msgpack_object* obj, size_t* n);
BOOL msgpack_unpack_array_min(msgpack_object* obj, size_t* n, size_t min);

BOOL msgpack_unpack_uint32(msgpack_object* obj, UINT32* d);

int msgpack_pack_cstr(msgpack_packer* pk, const char* cstr);
BOOL msgpack_unpack_cstr(msgpack_object* obj, char** cstr);

void msgpack_stream_packer_init(msgpack_packer* pk, wStream* s);
int msgpack_stream_write(wStream* s, const BYTE* buf, size_t len);

#endif /* FREERDS_RPC_INTERNAL_H */


