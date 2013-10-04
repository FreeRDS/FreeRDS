
#ifndef CALL_H_
#define CALL_H_

#include <string>
#include <stdint.h>
#include <winpr/synch.h>

namespace freerds{
	namespace sessionmanager{
		namespace call{
			class Call{

			public:
				Call();
				virtual ~Call();

				virtual unsigned long getCallType() = 0;
				virtual unsigned long getDerivedType() = 0;

				void setTag(uint32_t tag);
				uint32_t getTag();

				uint32_t getResult();
				std::string getErrorDescription();

			private:
				uint32_t mTag;
			protected:
				std::string mEncodedRequest;
				std::string mEncodedResponse;

				uint32_t mResult;
				// this is used if result ist not 0
				std::string mErrorDescription;
			};
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif
