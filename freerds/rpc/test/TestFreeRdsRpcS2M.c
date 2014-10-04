
#include <freerds/rpc.h>

int TestFreeRdsRpcS2M(int argc, char* argv[])
{
	wStream* s;
	UINT32 msgType;
	FDSAPI_START_SESSION_REQUEST request;

	request.SessionId = 123;
	request.User = "User";
	request.Domain = NULL;
	request.Password = "Password";

	s = Stream_New(NULL, 8192);

	if (!s)
		return 1;

	msgType = FDSAPI_START_SESSION_REQUEST_ID;

	s = freerds_rpc_msg_pack(msgType, &request, s);

	if (!s)
		return 1;

	ZeroMemory(&request, sizeof(request));

	freerds_rpc_msg_unpack(msgType, &request, Stream_Buffer(s), Stream_Length(s));

	fprintf(stderr, "SessionId: %d User: %s Domain: %s Password: %s\n",
			request.SessionId, request.User, request.Domain, request.Password);

	freerds_rpc_msg_free(msgType, &request);

	Stream_Free(s, TRUE);

	return 0;
}
