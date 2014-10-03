/**
 * Signaling queue template
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

#ifndef SIGNALINGQUEUE_H_
#define SIGNALINGQUEUE_H_

#include <list>
#include <winpr/synch.h>

template<typename QueueElement> class SignalingQueue
{
public:
	SignalingQueue()
	{
		if (!InitializeCriticalSectionAndSpinCount(&mCSection, 0x00000400))
		{
			// todo report error
		}

		mSignalHandle = CreateEvent(NULL,TRUE,FALSE,NULL);
	}

	~SignalingQueue()
	{
		CloseHandle(mSignalHandle);
		DeleteCriticalSection(&mCSection);
	}

	HANDLE getSignalHandle()
	{
		return mSignalHandle;
	}

	void addElement(QueueElement element)
	{
		EnterCriticalSection(&mCSection);
		mlist.push_back(element);
		SetEvent(mSignalHandle);
		LeaveCriticalSection(&mCSection);
	}

	void lockQueue()
	{
		EnterCriticalSection(&mCSection);
	}

	void resetEventAndLockQueue()
	{
		ResetEvent(mSignalHandle);
		EnterCriticalSection(&mCSection);
	}

	QueueElement getElementLockFree()
	{
		QueueElement element;
		if (mlist.empty())
			return QueueElement();
		element = mlist.front();
		mlist.pop_front();
		return element;
	}

	void unlockQueue()
	{
		LeaveCriticalSection(&mCSection);
	}

private:
	HANDLE mSignalHandle;
	CRITICAL_SECTION mCSection;
	std::list<QueueElement> mlist;
};

#endif /* SIGNALINGQUEUE_H_ */
