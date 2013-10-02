
#ifndef CALL_H_
#define CALL_H_

#include <string>
#include <stdint.h>

namespace freerds{
	namespace sessionmanager{
		namespace call{
			class Call{

			public:
				Call();
				virtual ~Call();

				virtual unsigned long getCallType() = 0;

				virtual int decodeRequest() = 0;
				virtual int encodeResponse() = 0;

				virtual int doStuff() = 0;

				void setEncodedRequest(std::string encodedRequest);
				std::string getEncodedResponse();

				void setTag(uint32_t tag);
				uint32_t getTag();

				uint32_t getResult();

			private:
				uint32_t mTag;
			protected:
				std::string mEncodedRequest;
				std::string mEncodedResponse;

				uint32_t mResult;


			};
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif
