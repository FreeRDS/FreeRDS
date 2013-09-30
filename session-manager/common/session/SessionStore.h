#ifndef SESSIONSTORE_H_
#define SESSIONSTORE_H_

#include <config.h>

namespace freeRDS{
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

namespace sessionNS = freeRDS::sessionmanager::session;

#endif
