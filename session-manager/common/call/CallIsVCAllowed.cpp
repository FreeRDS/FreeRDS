#include "CallIsVCAllowed.h"
#include <ICP.pb.h>

using freerds::icp::IsChannelAllowedRequest;
using freerds::icp::IsChannelAllowedResponse;

namespace freerds{
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
			IsChannelAllowedRequest req;
			if (!req.ParseFromString(mEncodedRequest)) {
				// failed to parse
				mResult = 1;// will report error with answer
				return -1;
			}
			mVirtualChannelName = req.channelname();
			return 0;
		};

		int CallIsVCAllowed::encodeResponse() {
			// encode protocol buffers
			IsChannelAllowedResponse resp;
			resp.set_channelallowed(mVirtualChannelAllowed);

			if (resp.SerializeToString(&mEncodedResponse)) {
				// failed to serialize
				mResult = 1;
				return -1;
			}
			return 0;
		};

		int CallIsVCAllowed::doStuff() {
			// find out if Virtual Channel is allowed
			mVirtualChannelAllowed = true;
			return 0;
		}


		}
	}
}

