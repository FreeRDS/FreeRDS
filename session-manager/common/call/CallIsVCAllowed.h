
#ifndef CALL_IS_VIRTUAL_CHANNEL_ALLOWED_H_
#define CALL_IS_VIRTUAL_CHANNEL_ALLOWED_H_
#include "CallFactory.h"
#include <string>
#include "Call.h"


namespace freeRDS{
	namespace sessionmanager{
		namespace call{
			class CallIsVCAllowed: public Call{

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

namespace callNS = freeRDS::sessionmanager::call;

#endif
