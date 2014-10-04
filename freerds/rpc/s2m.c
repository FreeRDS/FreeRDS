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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>
#include <winpr/stream.h>

#include "rpc.h"

#include <freerds/rpc.h>

/* FDSAPI_START_SESSION_REQUEST */

static wStream* freerds_pack_start_session_request(FDSAPI_START_SESSION_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 4);
	msgpack_pack_uint32(&pk, request->SessionId);
	msgpack_pack_cstr(&pk, request->User);
	msgpack_pack_cstr(&pk, request->Domain);
	msgpack_pack_cstr(&pk, request->Password);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_start_session_request(FDSAPI_START_SESSION_REQUEST* request, const BYTE* buffer, UINT32 size)
{
	size_t count;
	msgpack_unpacked pk;
	msgpack_object* obj;
	msgpack_object* root;

	msgpack_unpacked_init(&pk);

	if (!msgpack_unpack_next(&pk, (const char*) buffer, size, NULL))
		return FALSE;

	root = &(pk.data);
	obj = root->via.array.ptr;

	if (!msgpack_unpack_array_min(root, &count, 4))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->SessionId)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->User)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->Domain)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->Password)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_start_session_request(FDSAPI_START_SESSION_REQUEST* request)
{
	free(request->User);
	free(request->Domain);
	free(request->Password);
}

static RDS_RPC_PACK_FUNC g_FDSAPI_START_SESSION_REQUEST =
{
	FDSAPI_START_SESSION_REQUEST_ID,
	(pRdsRpcPack) freerds_pack_start_session_request,
	(pRdsRpcUnpack) freerds_unpack_start_session_request,
	(pRdsRpcFree) freerds_free_start_session_request
};

/* FDSAPI_START_SESSION_RESPONSE */

static wStream* freerds_pack_start_session_response(FDSAPI_START_SESSION_RESPONSE* response, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint32(&pk, response->status);
	msgpack_pack_cstr(&pk, response->ServiceEndpoint);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_start_session_response(FDSAPI_START_SESSION_RESPONSE* response, const BYTE* buffer, UINT32 size)
{
	size_t count;
	msgpack_unpacked pk;
	msgpack_object* obj;
	msgpack_object* root;

	msgpack_unpacked_init(&pk);

	if (!msgpack_unpack_next(&pk, (const char*) buffer, size, NULL))
		return FALSE;

	root = &(pk.data);
	obj = root->via.array.ptr;

	if (!msgpack_unpack_array_min(root, &count, 2))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(response->status)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(response->ServiceEndpoint)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_start_session_response(FDSAPI_START_SESSION_RESPONSE* response)
{
	free(response->ServiceEndpoint);
}

static RDS_RPC_PACK_FUNC g_FDSAPI_START_SESSION_RESPONSE =
{
	FDSAPI_START_SESSION_RESPONSE_ID,
	(pRdsRpcPack) freerds_pack_start_session_response,
	(pRdsRpcUnpack) freerds_unpack_start_session_response,
	(pRdsRpcFree) freerds_free_start_session_response
};

/* FDSAPI_END_SESSION_REQUEST */

static wStream* freerds_pack_end_session_request(FDSAPI_END_SESSION_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 1);
	msgpack_pack_uint32(&pk, request->SessionId);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_end_session_request(FDSAPI_END_SESSION_REQUEST* request, const BYTE* buffer, UINT32 size)
{
	size_t count;
	msgpack_unpacked pk;
	msgpack_object* obj;
	msgpack_object* root;

	msgpack_unpacked_init(&pk);

	if (!msgpack_unpack_next(&pk, (const char*) buffer, size, NULL))
		return FALSE;

	root = &(pk.data);
	obj = root->via.array.ptr;

	if (!msgpack_unpack_array_min(root, &count, 1))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->SessionId)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_end_session_request(FDSAPI_END_SESSION_REQUEST* request)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_END_SESSION_REQUEST =
{
	FDSAPI_END_SESSION_REQUEST_ID,
	(pRdsRpcPack) freerds_pack_end_session_request,
	(pRdsRpcUnpack) freerds_unpack_end_session_request,
	(pRdsRpcFree) freerds_free_end_session_request
};

/* FDSAPI_END_SESSION_RESPONSE */

static wStream* freerds_pack_end_session_response(FDSAPI_END_SESSION_RESPONSE* response, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint32(&pk, response->status);
	msgpack_pack_uint32(&pk, response->SessionId);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_end_session_response(FDSAPI_END_SESSION_RESPONSE* response, const BYTE* buffer, UINT32 size)
{
	size_t count;
	msgpack_unpacked pk;
	msgpack_object* obj;
	msgpack_object* root;

	msgpack_unpacked_init(&pk);

	if (!msgpack_unpack_next(&pk, (const char*) buffer, size, NULL))
		return FALSE;

	root = &(pk.data);
	obj = root->via.array.ptr;

	if (!msgpack_unpack_array_min(root, &count, 2))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(response->status)))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(response->SessionId)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_end_session_response(FDSAPI_END_SESSION_RESPONSE* response)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_END_SESSION_RESPONSE =
{
	FDSAPI_END_SESSION_RESPONSE_ID,
	(pRdsRpcPack) freerds_pack_end_session_response,
	(pRdsRpcUnpack) freerds_unpack_end_session_response,
	(pRdsRpcFree) freerds_free_end_session_response
};

/* Function Table */

RDS_RPC_PACK_FUNC* g_S2M_FUNCS[] =
{
	&g_FDSAPI_START_SESSION_REQUEST,
	&g_FDSAPI_START_SESSION_RESPONSE,
	&g_FDSAPI_END_SESSION_REQUEST,
	&g_FDSAPI_END_SESSION_RESPONSE,
	NULL
};
