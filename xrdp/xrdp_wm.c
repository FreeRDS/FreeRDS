/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
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
 * simple window manager
 */

#include "xrdp.h"
#include "log.h"

xrdpWm* xrdp_wm_create(xrdpSession* session)
{
	xrdpWm* self = (xrdpWm*) NULL;

	self = (xrdpWm*) g_malloc(sizeof(xrdpWm), 1);
	self->session = session;

	self->mm = xrdp_mm_create(session);
	xrdp_mm_connect(self->mm);

	return self;
}

void xrdp_wm_delete(xrdpWm* self)
{
	if (!self)
		return;

	xrdp_mm_delete(self->mm);

	free(self);
}

int xrdp_wm_get_event_handles(xrdpWm* self, HANDLE* events, DWORD* nCount)
{
	if (!self)
		return 0;

	return xrdp_mm_get_event_handles(self->mm, events, nCount);
}

int xrdp_wm_check_wait_objs(xrdpWm* self)
{
	int status;

	if (!self)
		return 0;

	status = xrdp_mm_check_wait_objs(self->mm);

	return status;
}

