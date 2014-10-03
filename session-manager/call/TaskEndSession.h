/**
 * Task for EndSession serivce call
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TASK_END_SESSION_
#define __TASK_END_SESSION_

#include <task/Task.h>

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		class TaskEndSession: public taskNS::Task {
		public:
			virtual void run();
			void setSessionId(long sessionId);
		private:
			void stopSession();
			long mSessionId;
		};

		typedef boost::shared_ptr<TaskEndSession> TaskEndSessionPtr;

		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif /* __TASK_MODULE_SHUTDOWN_ */
