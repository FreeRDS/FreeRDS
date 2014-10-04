
#include <freerds/rpc.h>

int TestFreeRdsRpcM2S(int argc, char* argv[])
{
	wStream* s;
	UINT32 msgType;
	FDSAPI_LOGON_USER_REQUEST request;

	request.ConnectionId = 321;
	request.User = "User";
	request.Domain = "Domain";
	request.Password = "Password";
	request.DesktopWidth = 1024;
	request.DesktopHeight = 768;
	request.ClientName = "ClientName";
	request.ClientAddress = "ClientAddress";
	request.ClientBuild = 6200;
	request.ClientProductId = 9000;
	request.ClientHardwareId = 45678;
	request.ClientProtocolType = 1;

	s = Stream_New(NULL, 512);

	if (!s)
		return 1;

	msgType = FDSAPI_LOGON_USER_REQUEST_ID;

	s = freerds_rpc_msg_pack(msgType, &request, s);

	if (!s)
		return 1;

	ZeroMemory(&request, sizeof(request));

	freerds_rpc_msg_unpack(msgType, &request, Stream_Buffer(s), Stream_Length(s));

	fprintf(stderr, "ConnectionId: %d User: %s Domain: %s Password: %s DesktopWidth: %d DesktopHeight: %d\n",
			request.ConnectionId, request.User, request.Domain, request.Password, request.DesktopWidth, request.DesktopHeight);

	fprintf(stderr, "ClientName: %s ClientAddress: %s ClientBuild: %d ClientProductId: %d ClientHardwareId: %d ClientProtocolType: %d\n",
			request.ClientName, request.ClientAddress, request.ClientBuild, request.ClientProductId, request.ClientHardwareId, request.ClientProtocolType);

	freerds_rpc_msg_free(msgType, &request);

	Stream_Free(s, TRUE);

	return 0;
}
