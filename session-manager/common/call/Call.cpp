#include "Call.h"
#include <string>

namespace freeRDS{
	namespace sessionmanager{
		namespace call{

		Call::Call():mTag(0),mResult(0) {

		};

		Call::~Call() {

		};

		void Call::setEncodedRequest(std::string encodedRequest) {
				mEncodedRequest = encodedRequest;
		}

		std::string Call::getEncodedResponse() {
			return mEncodedResponse;
		}

		void Call::setTag(uint32_t tag) {
			mTag = 0;
		}

		uint32_t Call::getTag() {
			return mTag;
		}

		uint32_t Call::getResult() {
			return mResult;
		}


		}
	}
}
