 /**
 * Rpc engine build upon google protocol buffers
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Executor.h"

#include <winpr/thread.h>
#include <winpr/wlog.h>
#include <appcontext/ApplicationContext.h>

namespace freerds {
namespace task {

		static wLog* logger_Executor = WLog_Get("freerds.task.Executor");

		Executor::Executor()
		{
			mhStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
			if (!InitializeCriticalSectionAndSpinCount(&mCSection, 0x00000400))
			{
				 WLog_Print(logger_Executor, WLOG_FATAL, "cannot init Executor critical section!");
			}
			mhServerThread = NULL;

		}

		Executor::~Executor()
		{
		}

		int Executor::start() {
			mhServerThread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE) Executor::execThread, (void*) this,
					CREATE_SUSPENDED, NULL);

			ResumeThread(mhServerThread);

			return 0;
		}

		int Executor::stop() {
			if (mhServerThread)
			{
				SetEvent(mhStopEvent);
				WaitForSingleObject(mhServerThread, INFINITE);
				CloseHandle(mhServerThread);
				mhServerThread = NULL;
			}

			return 0;
		}

		void Executor::runExecutor() {
			DWORD status;
			DWORD nCount;
			HANDLE queueHandle = mTasks.getSignalHandle();
			HANDLE events[2];

			nCount = 0;
			events[nCount++] = mhStopEvent;
			events[nCount++] = queueHandle;

			while (1)
			{
				status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

				if (WaitForSingleObject(mhStopEvent, 0) == WAIT_OBJECT_0)
				{
					break;
				}

				if (WaitForSingleObject(queueHandle, 0) == WAIT_OBJECT_0)
				{
					mTasks.resetEventAndLockQueue();
					Task * currentTask = mTasks.getElementLockFree();

					while (currentTask)
					{
						currentTask->run();
						delete currentTask;
						currentTask = mTasks.getElementLockFree();
					}

					mTasks.unlockQueue();
				}

			}
		}

		void Executor::addTask(Task* task) {
			mTasks.addElement(task);
		}

		void* Executor::execThread(void* arg) {
			Executor* executor;

			executor = (Executor*) arg;
			WLog_Print(logger_Executor, WLOG_INFO, "started Executor thread");

			executor->runExecutor();

			WLog_Print(logger_Executor, WLOG_INFO, "stopped Executor thread");
			return NULL;
		}



}
}
