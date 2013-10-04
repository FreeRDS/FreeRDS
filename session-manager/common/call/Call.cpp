#include "Call.h"
#include <string>
#include <winpr/handle.h>

namespace freerds{
	namespace sessionmanager{
		namespace call{

		Call::Call():mTag(0),mResult(0) {

		};

		Call::~Call() {
		};

		void Call::setTag(uint32_t tag) {
			mTag = 0;
		}

		uint32_t Call::getTag() {
			return mTag;
		}

		uint32_t Call::getResult() {
			return mResult;
		}

		std::string Call::getErrorDescription() {
			return mErrorDescription;
		}

		}
	}
}
