/**
 /**
 * Class for rpc call FdsApiVirtualChannelOpen (session manager to freerds)
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
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

#include "CallOutVirtualChannelOpen.h"
#include <appcontext/ApplicationContext.h>

namespace freerds
{
	CallOutVirtualChannelOpen::CallOutVirtualChannelOpen()
	: m_SessionId(0), m_ChannelHandle(0),
	  m_RequestId(FDSAPI_CHANNEL_ENDPOINT_OPEN_REQUEST_ID),
	  m_ResponseId(FDSAPI_CHANNEL_ENDPOINT_OPEN_RESPONSE_ID)
	{

	};

	CallOutVirtualChannelOpen::~CallOutVirtualChannelOpen()
	{

	};

	unsigned long CallOutVirtualChannelOpen::getCallType()
	{
		return m_RequestId;
	};

	int CallOutVirtualChannelOpen::encodeRequest()
	{
		wStream* s;

		m_Request.SessionId = m_SessionId;
		m_Request.ChannelName = (char*) m_ChannelName.c_str();

		s = freerds_rpc_msg_pack(m_RequestId, &m_Request, NULL);

		mEncodedRequest.assign((const char*) Stream_Buffer(s), Stream_Length(s));

		Stream_Free(s, TRUE);

		return 0;
	}

	int CallOutVirtualChannelOpen::decodeResponse()
	{
		BYTE* buffer;
		UINT32 length;

		buffer = (BYTE*) mEncodedResponse.data();
		length = (UINT32) mEncodedResponse.size();

		freerds_rpc_msg_unpack(m_ResponseId, &m_Response, buffer, length);

		m_ChannelHandle = m_Response.ChannelHandle;
		m_ChannelEndpoint = m_Response.ChannelEndpoint ? m_Response.ChannelEndpoint : "";

		freerds_rpc_msg_free(m_ResponseId, &m_Response);

		return 0;
	}

	void CallOutVirtualChannelOpen::setSessionID(UINT32 sessionId) {
		m_SessionId = sessionId;
	}

	void CallOutVirtualChannelOpen::setVirtualName(std::string channelName) {
		m_ChannelName = channelName;
	}

	std::string CallOutVirtualChannelOpen::getConnectionString() {
		return m_ChannelEndpoint;
	}
}
