#ifndef SESSIONSTORE_H_
#define SESSIONSTORE_H_

#include <config.h>

namespace freerds{
	namespace sessionmanager{
		namespace session{

		class SessionStore{
		public:
			SessionStore();
			~SessionStore();
		};

		}
	}
}

namespace sessionNS = freerds::sessionmanager::session;

#endif
