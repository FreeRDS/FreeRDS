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

#include "rpc.h"

#include <freerds/rpc.h>

/* FDSAPI_CHANNEL_ALLOWED_REQUEST */

static wStream* freerds_pack_channel_allowed_request(FDSAPI_CHANNEL_ALLOWED_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 1);
	msgpack_pack_cstr(&pk, request->ChannelName);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_channel_allowed_request(FDSAPI_CHANNEL_ALLOWED_REQUEST* request, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_cstr(obj++, &(request->ChannelName)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_channel_allowed_request(FDSAPI_CHANNEL_ALLOWED_REQUEST* request)
{
	free(request->ChannelName);
}

static RDS_RPC_PACK_FUNC g_FDSAPI_CHANNEL_ALLOWED_REQUEST =
{
	FDSAPI_CHANNEL_ALLOWED_REQUEST_ID,
	sizeof(FDSAPI_CHANNEL_ALLOWED_REQUEST),
	(pRdsRpcPack) freerds_pack_channel_allowed_request,
	(pRdsRpcUnpack) freerds_unpack_channel_allowed_request,
	(pRdsRpcFree) freerds_free_channel_allowed_request
};

/* FDSAPI_CHANNEL_ALLOWED_RESPONSE */

static wStream* freerds_pack_channel_allowed_response(FDSAPI_CHANNEL_ALLOWED_RESPONSE* response, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint32(&pk, response->status);
	msgpack_pack_bool(&pk, response->ChannelAllowed);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_channel_allowed_response(FDSAPI_CHANNEL_ALLOWED_RESPONSE* response, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_bool(obj++, &(response->ChannelAllowed)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_channel_allowed_response(FDSAPI_CHANNEL_ALLOWED_RESPONSE* response)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_CHANNEL_ALLOWED_RESPONSE =
{
	FDSAPI_CHANNEL_ALLOWED_RESPONSE_ID,
	sizeof(FDSAPI_CHANNEL_ALLOWED_RESPONSE),
	(pRdsRpcPack) freerds_pack_channel_allowed_response,
	(pRdsRpcUnpack) freerds_unpack_channel_allowed_response,
	(pRdsRpcFree) freerds_free_channel_allowed_response
};

/* FDSAPI_HEARTBEAT_REQUEST */

static wStream* freerds_pack_heartbeat_request(FDSAPI_HEARTBEAT_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 1);
	msgpack_pack_uint32(&pk, request->HeartbeatId);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_heartbeat_request(FDSAPI_HEARTBEAT_REQUEST* request, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(request->HeartbeatId)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_heartbeat_request(FDSAPI_HEARTBEAT_REQUEST* request)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_HEARTBEAT_REQUEST =
{
	FDSAPI_HEARTBEAT_REQUEST_ID,
	sizeof(FDSAPI_HEARTBEAT_REQUEST),
	(pRdsRpcPack) freerds_pack_heartbeat_request,
	(pRdsRpcUnpack) freerds_unpack_heartbeat_request,
	(pRdsRpcFree) freerds_free_heartbeat_request
};

/* FDSAPI_HEARTBEAT_RESPONSE */

static wStream* freerds_pack_heartbeat_response(FDSAPI_HEARTBEAT_RESPONSE* response, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint32(&pk, response->status);
	msgpack_pack_uint32(&pk, response->HeartbeatId);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_heartbeat_response(FDSAPI_HEARTBEAT_RESPONSE* response, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(response->HeartbeatId)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_heartbeat_response(FDSAPI_HEARTBEAT_RESPONSE* response)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_HEARTBEAT_RESPONSE =
{
	FDSAPI_HEARTBEAT_RESPONSE_ID,
	sizeof(FDSAPI_HEARTBEAT_RESPONSE),
	(pRdsRpcPack) freerds_pack_heartbeat_response,
	(pRdsRpcUnpack) freerds_unpack_heartbeat_response,
	(pRdsRpcFree) freerds_free_heartbeat_response
};

/* FDSAPI_LOGON_USER_REQUEST */

static wStream* freerds_pack_logon_user_request(FDSAPI_LOGON_USER_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 13);
	msgpack_pack_uint32(&pk, request->ConnectionId);
	msgpack_pack_cstr(&pk, request->User);
	msgpack_pack_cstr(&pk, request->Domain);
	msgpack_pack_cstr(&pk, request->Password);
	msgpack_pack_uint32(&pk, request->DesktopWidth);
	msgpack_pack_uint32(&pk, request->DesktopHeight);
	msgpack_pack_uint32(&pk, request->ColorDepth);
	msgpack_pack_cstr(&pk, request->ClientName);
	msgpack_pack_cstr(&pk, request->ClientAddress);
	msgpack_pack_uint32(&pk, request->ClientBuild);
	msgpack_pack_uint32(&pk, request->ClientProductId);
	msgpack_pack_uint32(&pk, request->ClientHardwareId);
	msgpack_pack_uint32(&pk, request->ClientProtocolType);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_logon_user_request(FDSAPI_LOGON_USER_REQUEST* request, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_array_min(root, &count, 13))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->ConnectionId)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->User)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->Domain)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->Password)))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->DesktopWidth)))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->DesktopHeight)))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->ColorDepth)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->ClientName)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->ClientAddress)))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->ClientBuild)))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->ClientProductId)))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->ClientHardwareId)))
		return FALSE;

	if (!msgpack_unpack_uint32(obj++, &(request->ClientProtocolType)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_logon_user_request(FDSAPI_LOGON_USER_REQUEST* request)
{
	free(request->User);
	free(request->Domain);
	free(request->Password);
	free(request->ClientName);
	free(request->ClientAddress);
}

static RDS_RPC_PACK_FUNC g_FDSAPI_LOGON_USER_REQUEST =
{
	FDSAPI_LOGON_USER_REQUEST_ID,
	sizeof(FDSAPI_LOGON_USER_REQUEST),
	(pRdsRpcPack) freerds_pack_logon_user_request,
	(pRdsRpcUnpack) freerds_unpack_logon_user_request,
	(pRdsRpcFree) freerds_free_logon_user_request
};

/* FDSAPI_LOGON_USER_RESPONSE */

static wStream* freerds_pack_logon_user_response(FDSAPI_LOGON_USER_RESPONSE* response, wStream* s)
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

static BOOL freerds_unpack_logon_user_response(FDSAPI_LOGON_USER_RESPONSE* response, const BYTE* buffer, UINT32 size)
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

static void freerds_free_logon_user_response(FDSAPI_LOGON_USER_RESPONSE* response)
{
	free(response->ServiceEndpoint);
}

static RDS_RPC_PACK_FUNC g_FDSAPI_LOGON_USER_RESPONSE =
{
	FDSAPI_LOGON_USER_RESPONSE_ID,
	sizeof(FDSAPI_LOGON_USER_RESPONSE),
	(pRdsRpcPack) freerds_pack_logon_user_response,
	(pRdsRpcUnpack) freerds_unpack_logon_user_response,
	(pRdsRpcFree) freerds_free_logon_user_response
};

/* FDSAPI_LOGOFF_USER_REQUEST */

static wStream* freerds_pack_logoff_user_request(FDSAPI_LOGOFF_USER_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 1);
	msgpack_pack_uint32(&pk, request->ConnectionId);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_logoff_user_request(FDSAPI_LOGOFF_USER_REQUEST* request, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(request->ConnectionId)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_logoff_user_request(FDSAPI_LOGOFF_USER_REQUEST* request)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_LOGOFF_USER_REQUEST =
{
	FDSAPI_LOGOFF_USER_REQUEST_ID,
	sizeof(FDSAPI_LOGOFF_USER_REQUEST),
	(pRdsRpcPack) freerds_pack_logoff_user_request,
	(pRdsRpcUnpack) freerds_unpack_logoff_user_request,
	(pRdsRpcFree) freerds_free_logoff_user_request
};

/* FDSAPI_LOGOFF_USER_RESPONSE */

static wStream* freerds_pack_logoff_user_response(FDSAPI_LOGOFF_USER_RESPONSE* response, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint32(&pk, response->status);
	msgpack_pack_uint32(&pk, response->ConnectionId);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_logoff_user_response(FDSAPI_LOGOFF_USER_RESPONSE* response, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(response->ConnectionId)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_logoff_user_response(FDSAPI_LOGOFF_USER_RESPONSE* response)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_LOGOFF_USER_RESPONSE =
{
	FDSAPI_LOGOFF_USER_RESPONSE_ID,
	sizeof(FDSAPI_LOGOFF_USER_RESPONSE),
	(pRdsRpcPack) freerds_pack_logoff_user_response,
	(pRdsRpcUnpack) freerds_unpack_logoff_user_response,
	(pRdsRpcFree) freerds_free_logoff_user_response
};

/* FDSAPI_DISCONNECT_USER_REQUEST */

static wStream* freerds_pack_disconnect_user_request(FDSAPI_DISCONNECT_USER_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 1);
	msgpack_pack_uint32(&pk, request->ConnectionId);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_disconnect_user_request(FDSAPI_DISCONNECT_USER_REQUEST* request, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(request->ConnectionId)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_disconnect_user_request(FDSAPI_DISCONNECT_USER_REQUEST* request)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_DISCONNECT_USER_REQUEST =
{
	FDSAPI_DISCONNECT_USER_REQUEST_ID,
	sizeof(FDSAPI_DISCONNECT_USER_REQUEST),
	(pRdsRpcPack) freerds_pack_disconnect_user_request,
	(pRdsRpcUnpack) freerds_unpack_disconnect_user_request,
	(pRdsRpcFree) freerds_free_disconnect_user_request
};

/* FDSAPI_DISCONNECT_USER_RESPONSE */

static wStream* freerds_pack_disconnect_user_response(FDSAPI_DISCONNECT_USER_RESPONSE* response, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint32(&pk, response->status);
	msgpack_pack_uint32(&pk, response->ConnectionId);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_disconnect_user_response(FDSAPI_DISCONNECT_USER_RESPONSE* response, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(response->ConnectionId)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_disconnect_user_response(FDSAPI_DISCONNECT_USER_RESPONSE* response)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_DISCONNECT_USER_RESPONSE =
{
	FDSAPI_DISCONNECT_USER_RESPONSE_ID,
	sizeof(FDSAPI_DISCONNECT_USER_RESPONSE),
	(pRdsRpcPack) freerds_pack_disconnect_user_response,
	(pRdsRpcUnpack) freerds_unpack_disconnect_user_response,
	(pRdsRpcFree) freerds_free_disconnect_user_response
};

/* FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST */

static wStream* freerds_pack_switch_service_endpoint_request(FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint32(&pk, request->ConnectionId);
	msgpack_pack_cstr(&pk, request->ServiceEndpoint);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_switch_service_endpoint_request(FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST* request, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(request->ConnectionId)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->ServiceEndpoint)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_switch_service_endpoint_request(FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST* request)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST =
{
	FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST_ID,
	sizeof(FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST),
	(pRdsRpcPack) freerds_pack_switch_service_endpoint_request,
	(pRdsRpcUnpack) freerds_unpack_switch_service_endpoint_request,
	(pRdsRpcFree) freerds_free_switch_service_endpoint_request
};

/* FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE */

static wStream* freerds_pack_switch_service_endpoint_response(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE* response, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 1);
	msgpack_pack_uint32(&pk, response->status);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_switch_service_endpoint_response(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE* response, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(response->status)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_switch_service_endpoint_response(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE* response)
{

}

static RDS_RPC_PACK_FUNC g_FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE =
{
	FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE_ID,
	sizeof(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE),
	(pRdsRpcPack) freerds_pack_switch_service_endpoint_response,
	(pRdsRpcUnpack) freerds_unpack_switch_service_endpoint_response,
	(pRdsRpcFree) freerds_free_switch_service_endpoint_response
};

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
	sizeof(FDSAPI_START_SESSION_REQUEST),
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
	sizeof(FDSAPI_START_SESSION_RESPONSE),
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
	sizeof(FDSAPI_END_SESSION_REQUEST),
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
	sizeof(FDSAPI_END_SESSION_RESPONSE),
	(pRdsRpcPack) freerds_pack_end_session_response,
	(pRdsRpcUnpack) freerds_unpack_end_session_response,
	(pRdsRpcFree) freerds_free_end_session_response
};

/* FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST */

static wStream* freerds_pack_channel_endpoint_open_request(FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST* request, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 2);
	msgpack_pack_uint32(&pk, request->SessionId);
	msgpack_pack_cstr(&pk, request->ChannelName);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_channel_endpoint_open_request(FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST* request, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint32(obj++, &(request->SessionId)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(request->ChannelName)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_channel_endpoint_open_request(FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST* request)
{
	free(request->ChannelName);
}

static RDS_RPC_PACK_FUNC g_FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST =
{
	FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST_ID,
	sizeof(FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST),
	(pRdsRpcPack) freerds_pack_channel_endpoint_open_request,
	(pRdsRpcUnpack) freerds_unpack_channel_endpoint_open_request,
	(pRdsRpcFree) freerds_free_channel_endpoint_open_request
};

/* FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE */

static wStream* freerds_pack_channel_endpoint_open_response(FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE* response, wStream* s)
{
	msgpack_packer pk;

	STREAM_PACK_PREPARE(s);

	msgpack_stream_packer_init(&pk, s);

	msgpack_pack_array(&pk, 3);
	msgpack_pack_uint32(&pk, response->status);
	msgpack_pack_uint64(&pk, response->ChannelHandle);
	msgpack_pack_cstr(&pk, response->ChannelEndpoint);

	STREAM_PACK_FINALIZE(s);

	return s;
}

static BOOL freerds_unpack_channel_endpoint_open_response(FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE* response, const BYTE* buffer, UINT32 size)
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

	if (!msgpack_unpack_uint64(obj++, &(response->ChannelHandle)))
		return FALSE;

	if (!msgpack_unpack_cstr(obj++, &(response->ChannelEndpoint)))
		return FALSE;

	msgpack_unpacked_destroy(&pk);

	return TRUE;
}

static void freerds_free_channel_endpoint_open_response(FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE* response)
{
	free(response->ChannelEndpoint);
}

static RDS_RPC_PACK_FUNC g_FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE =
{
	FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE_ID,
	sizeof(FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE),
	(pRdsRpcPack) freerds_pack_channel_endpoint_open_response,
	(pRdsRpcUnpack) freerds_unpack_channel_endpoint_open_response,
	(pRdsRpcFree) freerds_free_channel_endpoint_open_response
};

/* Function Table */

RDS_RPC_PACK_FUNC* g_MSG_FUNCS[] =
{
	&g_FDSAPI_CHANNEL_ALLOWED_REQUEST,
	&g_FDSAPI_CHANNEL_ALLOWED_RESPONSE,
	&g_FDSAPI_HEARTBEAT_REQUEST,
	&g_FDSAPI_HEARTBEAT_RESPONSE,
	&g_FDSAPI_LOGON_USER_REQUEST,
	&g_FDSAPI_LOGOFF_USER_REQUEST,
	&g_FDSAPI_LOGOFF_USER_RESPONSE,
	&g_FDSAPI_DISCONNECT_USER_REQUEST,
	&g_FDSAPI_DISCONNECT_USER_RESPONSE,
	&g_FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST,
	&g_FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE,
	&g_FDSAPI_LOGON_USER_RESPONSE,
	&g_FDSAPI_START_SESSION_REQUEST,
	&g_FDSAPI_START_SESSION_RESPONSE,
	&g_FDSAPI_END_SESSION_REQUEST,
	&g_FDSAPI_END_SESSION_RESPONSE,
	&g_FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST,
	&g_FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE,
	NULL
};
