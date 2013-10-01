#include "CallIsVCAllowed.h"

namespace freeRDS{
	namespace sessionmanager{
		namespace call{

		CallIsVCAllowed::CallIsVCAllowed() {

		};

		CallIsVCAllowed::~CallIsVCAllowed() {

		};

		unsigned long CallIsVCAllowed::getCallType() {
			return 1;
		};

		int CallIsVCAllowed::decodeRequest() {
				// decode protocol buffers
		};

		int CallIsVCAllowed::encodeResponse() {
				// encode protocol buffers
		};

		int CallIsVCAllowed::doStuff() {
			// find out if Virtual Channel is allowed
		}


		}
	}
}

