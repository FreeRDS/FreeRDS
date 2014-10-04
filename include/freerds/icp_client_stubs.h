/**
 * Freerds internal communication protocol
 * Client stubs
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

#ifndef _ICP_CLIENT_STUBS_H
#define _ICP_CLIENT_STUBS_H

#include <winpr/wtypes.h>

#include <freerds/rpc.h>

int freerds_icp_IsChannelAllowed(FDSAPI_CHANNEL_ALLOWED_REQUEST* pRequest, FDSAPI_CHANNEL_ALLOWED_RESPONSE* pResponse);
int freerds_icp_DisconnectUserSession(FDSAPI_DISCONNECT_USER_REQUEST* pRequest, FDSAPI_DISCONNECT_USER_RESPONSE* pResponse);
int freerds_icp_LogOffUserSession(FDSAPI_LOGOFF_USER_REQUEST* pRequest, FDSAPI_LOGOFF_USER_RESPONSE* pResponse);
int freerds_icp_LogonUser(FDSAPI_LOGON_USER_REQUEST* pRequest, FDSAPI_LOGON_USER_RESPONSE* pResponse);

int freerds_icp_LogoffUserResponse(FDSAPI_LOGOFF_USER_RESPONSE* pResponse);
int freerds_icp_SwitchServiceEndpointResponse(FDSAPI_SWITCH_SERVICE_ENDPOINT_RESPONSE* pResponse);

#endif // _ICP_CLIENT_STUBS_H
