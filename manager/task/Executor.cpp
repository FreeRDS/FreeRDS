 /**
 * Rpc engine build upon google protocol buffers
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Executor.h"

#include <winpr/thread.h>
#include <winpr/wlog.h>
#include <appcontext/ApplicationContext.h>
#include <utils/CSGuard.h>

#include <functional>

namespace freerds {
namespace task {

		static wLog* logger_Executor = WLog_Get("freerds.task.Executor");

		Executor::Executor()
		{
			mhStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
			mhTaskThreadStarted = CreateEvent(NULL,TRUE,FALSE,NULL);
			mhStopThreads = CreateEvent(NULL,TRUE,FALSE,NULL);

			if (!InitializeCriticalSectionAndSpinCount(&mCSection, 0x00000400))
			{
				WLog_Print(logger_Executor, WLOG_FATAL, "cannot init Executor critical section!");
			}

			mhServerThread = NULL;
			mRunning = false;

		}

		Executor::~Executor()
		{
		}

		int Executor::start()
		{
			CSGuard guard(&mCSection);

			if (mRunning)
			{
				WLog_Print(logger_Executor, WLOG_ERROR, "Executor Thread already started!");
				return 1;
			}

			mhServerThread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE) Executor::execThread, (void*) this,
					CREATE_SUSPENDED, NULL);

			ResumeThread(mhServerThread);
			mRunning = true;

			return 0;
		}

		int Executor::stop()
		{
			CSGuard guard(&mCSection);

			mRunning = false;

			if (mhServerThread)
			{
				SetEvent(mhStopEvent);
				WaitForSingleObject(mhServerThread, INFINITE);
				CloseHandle(mhServerThread);
				mhServerThread = NULL;
			}
			else
			{
				WLog_Print(logger_Executor, WLOG_ERROR, "Executor was not started before.");
				return 1;
			}

			return 0;
		}

		void Executor::runExecutor()
		{
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
					SetEvent(mhStopThreads);
					// shut down all threads
					mTaskThreadList.remove_if(std::bind1st(std::mem_fun(&Executor::waitThreadHandles),this));
					break;
				}

				if (WaitForSingleObject(queueHandle, 0) == WAIT_OBJECT_0)
				{
					mTasks.resetEventAndLockQueue();
					TaskPtr currentTask = mTasks.getElementLockFree();

					while (currentTask)
					{
						if (!currentTask->isThreaded())
						{
							currentTask->run();
							currentTask.reset();
						}
						else
						{
							// start Task as thread

							currentTask->setHandles(mhStopThreads,mhTaskThreadStarted);
							HANDLE taskThread = CreateThread(NULL, 0,
									(LPTHREAD_START_ROUTINE) Executor::execTask, (void*) &currentTask,
									CREATE_SUSPENDED, NULL);

							ResumeThread(taskThread);
							mTaskThreadList.push_back(taskThread);
							WaitForSingleObject(mhTaskThreadStarted,INFINITE);
							ResetEvent(mhTaskThreadStarted);
							currentTask.reset();
						}
						currentTask = mTasks.getElementLockFree();
						mTaskThreadList.remove_if(std::bind1st(std::mem_fun(&Executor::checkThreadHandles),this));
					}

					mTasks.unlockQueue();
				}
			}
		}

		bool Executor::addTask(TaskPtr task)
		{
			CSGuard guard(&mCSection);

			if (mRunning) {
				mTasks.addElement(task);
				return true;
			} else {
				return false;
			}
		}

		void* Executor::execThread(void* arg)
		{
			Executor* executor;

			executor = (Executor*) arg;
			WLog_Print(logger_Executor, WLOG_INFO, "started Executor thread");

			executor->runExecutor();

			WLog_Print(logger_Executor, WLOG_INFO, "stopped Executor thread");
			return NULL;
		}

		void* Executor::execTask(void* arg)
		{
			TaskPtr *taskptr = static_cast<TaskPtr*>(arg);
			TaskPtr task = *taskptr;
			task->informStarted();

			WLog_Print(logger_Executor, WLOG_TRACE, "started Task thread");

			task->run();

			WLog_Print(logger_Executor, WLOG_TRACE, "stopped Task thread");
			return NULL;
		}

		bool Executor::checkThreadHandles(const HANDLE value) const
		{
			if (WaitForSingleObject(value,0) == WAIT_OBJECT_0)
				return true;

			return false;
		}

		bool Executor::waitThreadHandles(const HANDLE value) const
		{
			if (WaitForSingleObject(value,INFINITE) == WAIT_OBJECT_0)
				return true;

			return false;
		}
}
}
