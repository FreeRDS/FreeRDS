#include "Call.h"
#include <string>

namespace freeRDS{
	namespace sessionmanager{
		namespace call{

		Call::Call() {

		};

		Call::~Call() {

		};
		void Call::setEncodedRequest(std::string encodedRequest) {
				mEncodedRequest = encodedRequest;
		}

		std::string Call::getEncodedResponse() {
			return mEncodedResponse;
		}


		}
	}
}
