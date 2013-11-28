/**
 * Task for switching a Module
 *
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thinstuff.at>
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

#ifndef __TASK_MODULE_SWITCH_TO_
#define __TASK_MODULE_SWITCH_TO_

#include <task/Task.h>
#include <string>

namespace freerds
{
	namespace sessionmanager
	{
		namespace call
		{

		class TaskSwitchTo: public taskNS::Task {
		public:
			virtual void run();

			void setConnectionId(long connectionId);
			void setServiceEndpoint(std::string serviceEndpoint);

			void setOldSessionId(long sessionId);
			void setNewSessionId(long sessionId);

		private:
			long mConnectionId;
			std::string mServiceEndpoint;
			long mOldSessionId;
			long mNewSessionId;

		};
		}
	}
}

namespace callNS = freerds::sessionmanager::call;

#endif /* __TASK_MODULE_SWITCH_TO_ */
