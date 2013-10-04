#include "CallOut.h"
#include <string>
#include <winpr/handle.h>

namespace freerds{
	namespace sessionmanager{
		namespace call{

		CallOut::CallOut():mAnswer(NULL) {

		};

		CallOut::~CallOut() {
			if (mAnswer) {
				CloseHandle(mAnswer);
				mAnswer = NULL;
			}

		};

		std::string CallOut::getEncodedRequest() {
			return mEncodedRequest;
		}

		void CallOut::setEncodedeResponse(std::string encodedResponse){
			mEncodedResponse = encodedResponse;
		}

		void CallOut::initAnswerHandle() {
			mAnswer = CreateEvent(NULL,FALSE,FALSE,NULL);
		}

		HANDLE CallOut::getAnswerHandle() {
			return mAnswer;
		}

		unsigned long CallOut::getDerivedType() {
			return 2; // for all CallIns
		}



		}
	}
}
