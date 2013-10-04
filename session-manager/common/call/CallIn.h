/*
 * CallIn.h
 *
 *  Created on: Oct 4, 2013
 *      Author: retro
 */

#ifndef CALLIN_H_
#define CALLIN_H_

#include <call/Call.h>

namespace freerds{
	namespace sessionmanager{
		namespace call{

			class CallIn:public Call {

			public:
				CallIn();
				virtual ~CallIn();
				virtual unsigned long getDerivedType();

				void setEncodedRequest(std::string encodedRequest);
				virtual int decodeRequest() = 0;


				virtual int encodeResponse() = 0;
				std::string getEncodedResponse();

				virtual int doStuff() = 0;


			};

		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif /* CALLIN_H_ */
