/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
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
 *
 * thread calls
 */

#if !defined(THREAD_CALLS_H)
#define THREAD_CALLS_H

#include <winpr/wtypes.h>

int tc_thread_create(void* (*start_routine)(void*), void* arg);
LONG_PTR tc_get_threadid(void);
int tc_threadid_equal(LONG_PTR tid1, LONG_PTR tid2);
LONG_PTR tc_mutex_create(void);
void tc_mutex_delete(LONG_PTR mutex);
int tc_mutex_lock(LONG_PTR mutex);
int tc_mutex_unlock(LONG_PTR mutex);
LONG_PTR tc_sem_create(int init_count);
void tc_sem_delete(LONG_PTR sem);
int tc_sem_dec(LONG_PTR sem);
int tc_sem_inc(LONG_PTR sem);

#endif
