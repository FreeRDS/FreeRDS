
#ifndef CALL_H_
#define CALL_H_

#include <string>

namespace freeRDS{
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

			private:
				std::string mEncodedRequest;
				std::string mEncodedResponse;


			};
		}
	}
}

namespace callNS = freeRDS::sessionmanager::call;

#endif
