/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * CEF Module Service
 *
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#ifndef FREERDS_MODULE_CEF_SERVICE_H
#define FREERDS_MODULE_CEF_SERVICE_H

#include <winpr/crt.h>
#include <winpr/wlog.h>
#include <winpr/synch.h>
#include <winpr/thread.h>

#include <freerds/freerds.h>

typedef struct rds_context_cef rdsContextCef;

struct rds_context_cef
{
	wLog* log;
	DWORD SessionId;
	rdsService* service;

	int framebufferSize;
	RDS_FRAMEBUFFER framebuffer;
};

#endif /* FREERDS_MODULE_CEF_SERVICE_H */
