/**
 * Class for rpc call IsVirtualChannelAllowed (freerds to session manager)
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

#include "CallInIsVCAllowed.h"

using freerds::icp::IsChannelAllowedRequest;
using freerds::icp::IsChannelAllowedResponse;

namespace freerds
{
	namespace call
	{
		CallInIsVCAllowed::CallInIsVCAllowed()
		: mVirtualChannelAllowed(false),
		  m_RequestId(FDSAPI_CHANNEL_ALLOWED_REQUEST_ID), m_ResponseId(FDSAPI_CHANNEL_ALLOWED_RESPONSE_ID)
		{

		};

		CallInIsVCAllowed::~CallInIsVCAllowed()
		{

		};

		unsigned long CallInIsVCAllowed::getCallType()
		{
			return m_RequestId;
		};

		int CallInIsVCAllowed::decodeRequest()
		{
			BYTE* buffer;
			UINT32 length;

			buffer = (BYTE*) mEncodedRequest.data();
			length = (UINT32) mEncodedRequest.size();

			freerds_rpc_msg_unpack(m_RequestId, &m_Request, buffer, length);

			mVirtualChannelName = m_Request.ChannelName ? m_Request.ChannelName : "";

			freerds_rpc_msg_free(m_RequestId, &m_Request);

			return 0;
		};

		int CallInIsVCAllowed::encodeResponse()
		{
			wStream* s;

			m_Response.ChannelAllowed = mVirtualChannelAllowed ? TRUE : FALSE;

			s = freerds_rpc_msg_pack(m_ResponseId, &m_Response, NULL);

			mEncodedResponse.assign((const char*) Stream_Buffer(s), Stream_Length(s));

			Stream_Free(s, TRUE);

			return 0;
		};

		int CallInIsVCAllowed::doStuff()
		{
			mVirtualChannelAllowed = true;
			return 0;
		}
	}
}

