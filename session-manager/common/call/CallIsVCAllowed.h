
#ifndef CALL_IS_VIRTUAL_CHANNEL_ALLOWED_H_
#define CALL_IS_VIRTUAL_CHANNEL_ALLOWED_H_
#include "CallFactory.h"
#include <string>
#include "CallIn.h"


namespace freerds{
	namespace sessionmanager{
		namespace call{
			class CallIsVCAllowed: public CallIn{

			public:
				CallIsVCAllowed();
				virtual ~CallIsVCAllowed();

				virtual unsigned long getCallType();
				virtual int decodeRequest();
				virtual int encodeResponse();
				virtual int doStuff();



			private:
				std::string mVirtualChannelName;
				bool        mVirtualChannelAllowed;

			};

			FACTORY_REGISTER_DWORD(CallFactory,CallIsVCAllowed,1);
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif
