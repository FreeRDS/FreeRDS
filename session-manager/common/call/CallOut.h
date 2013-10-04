/*
 * CallOut.h
 *
 *  Created on: Oct 4, 2013
 *      Author: retro
 */

#ifndef CALLOUT_H_
#define CALLOUT_H_

#include <call/Call.h>

namespace freerds{
	namespace sessionmanager{
		namespace call{

			class CallOut:public Call {

				CallOut();
				~CallOut();
				virtual unsigned long getDerivedType();


				virtual int encodeRequest() = 0;
				std::string getEncodedRequest();

				void setEncodedeResponse(std::string encodedResponse);
				virtual int decodeResponse() = 0;

				void   initAnswerHandle();
				HANDLE getAnswerHandle();

			private :
				HANDLE mAnswer;


			};

		}
	}
}

namespace callNS = freerds::sessionmanager::call;


#endif /* CALLOUT_H_ */
