/**
 * Class for ping rpc call (freerds to session manager)
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CallInPing.h"

namespace freerds
{
	CallInPing::CallInPing()
	: m_RequestId(FDSAPI_HEARTBEAT_REQUEST_ID), m_ResponseId(FDSAPI_HEARTBEAT_RESPONSE_ID)
	{

	};

	CallInPing::~CallInPing()
	{

	};

	unsigned long CallInPing::getCallType()
	{
		return m_RequestId;
	};

	int CallInPing::decodeRequest()
	{
		BYTE* buffer;
		UINT32 length;

		buffer = (BYTE*) mEncodedRequest.data();
		length = (UINT32) mEncodedRequest.size();

		freerds_rpc_msg_unpack(m_RequestId, &m_Request, buffer, length);

		freerds_rpc_msg_free(m_RequestId, &m_Request);

		return 0;
	};

	int CallInPing::encodeResponse()
	{
		wStream* s;

		m_Response.HeartbeatId = m_Request.HeartbeatId;

		s = freerds_rpc_msg_pack(m_ResponseId, &m_Response, NULL);

		mEncodedResponse.assign((const char*) Stream_Buffer(s), Stream_Length(s));

		Stream_Free(s, TRUE);

		return 0;
	};

	int CallInPing::doStuff()
	{
		return 0;
	}
}
