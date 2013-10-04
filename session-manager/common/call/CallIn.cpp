#include "CallIn.h"
#include <string>
#include <winpr/handle.h>

namespace freerds{
	namespace sessionmanager{
		namespace call{

		CallIn::CallIn() {

		};

		CallIn::~CallIn() {
		};

		void CallIn::setEncodedRequest(std::string encodedRequest) {
			mEncodedRequest = encodedRequest;
		}

		std::string CallIn::getEncodedResponse() {
			return mEncodedResponse;
		}

		unsigned long CallIn::getDerivedType() {
			return 1; // for all CallIns
		}


		}
	}
}
