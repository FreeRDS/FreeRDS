/**
 * Class for rpc call SwitchTo (session manager to freerds)
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

#ifndef CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_
#define CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_

#include <string>

#include "CallOut.h"

namespace freerds
{
	class CallOutSwitchTo: public CallOut
	{
	public:
		CallOutSwitchTo();
		virtual ~CallOutSwitchTo();

		virtual unsigned long getCallType();

		virtual int encodeRequest();
		virtual int decodeResponse();

		void setConnectionId(UINT32 connectionId);
		void setServiceEndpoint(std::string serviceEndpoint);

		bool isSuccess();

	private:
		bool m_Success;
		long m_ConnectionId;
		std::string m_ServiceEndpoint;

		UINT32 m_RequestId;
		FDSAPI_SWITCH_SERVICE_ENDPOINT_REQUEST m_Request;

		UINT32 m_ResponseId;
		FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE m_Response;
	};
}

#endif //CALL_OUT_FDS_API_VIRTUAL_CHANNEL_OPEN_H_
