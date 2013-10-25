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
 * module manager
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xrdp.h"

#include <pwd.h>
#include <grp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <winpr/crt.h>
#include <winpr/pipe.h>
#include <winpr/path.h>
#include <winpr/file.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <winpr/stream.h>
#include <winpr/library.h>
#include <winpr/sspicli.h>
#include <winpr/environment.h>

#include <freerdp/freerdp.h>

extern char* RdsModuleName;

int xrdp_client_get_event_handles(rdsModuleConnector* connector, HANDLE* events, DWORD* nCount)
{
	if (connector)
	{
		if (connector->ServerQueue)
		{
			events[*nCount] = MessageQueue_Event(connector->ServerQueue);
			(*nCount)++;
		}
	}

	return 0;
}

int xrdp_client_check_event_handles(rdsModuleConnector* connector)
{
	int status = 0;


	if (!connector)
		return 0;

	while (WaitForSingleObject(MessageQueue_Event(connector->ServerQueue), 0) == WAIT_OBJECT_0)
	{
		status = xrdp_message_server_queue_process_pending_messages(connector);
	}

	return status;
}

pRdsModuleEntry freerds_load_dynamic_module(const char* name)
{
	char* lowerName;
	HINSTANCE library;
	char moduleFileName[256];
	pRdsModuleEntry moduleEntry;

	lowerName = _strdup(name);
	CharLowerA(lowerName);

	sprintf_s(moduleFileName, sizeof(moduleFileName), RDS_LIB_PATH "/libfreerds-%s.so", lowerName);

	free(lowerName);

	library = LoadLibraryA(moduleFileName);

	if (!library)
		return NULL;

	moduleEntry = GetProcAddress(library, RDS_MODULE_ENTRY_POINT_NAME);

	if (moduleEntry)
		return moduleEntry;

	FreeLibrary(library);

	return NULL;
}

rdsModuleConnector* xrdp_module_new(rdsConnection* connection)
{
	int error_code;
	int auth_status;
	rdpSettings* settings;
	rdsModuleConnector* connector;
	pRdsModuleEntry moduleEntry;
	RDS_MODULE_ENTRY_POINTS EntryPoints;

	ZeroMemory(&EntryPoints, sizeof(EntryPoints));
	EntryPoints.Version = RDS_MODULE_INTERFACE_VERSION;
	EntryPoints.Size = sizeof(EntryPoints);

	if (!RdsModuleName)
		RdsModuleName = _strdup("X11");

	moduleEntry = freerds_load_dynamic_module(RdsModuleName);

	if (moduleEntry)
	{
		moduleEntry(&EntryPoints);
	}
	else
	{
		fprintf(stderr, "failed to load module");
		return NULL;
	}

	settings = connection->settings;

	auth_status = xrdp_authenticate(settings->Username, settings->Password, &error_code);

	connector = (rdsModuleConnector*) malloc(EntryPoints.ContextSize);
	ZeroMemory(connector, EntryPoints.ContextSize);

	connector->Size = EntryPoints.ContextSize;
	connector->SessionId = 10;

	connector->connection = connection;
	connector->settings = connection->settings;

	connector->pEntryPoints = (RDS_MODULE_ENTRY_POINTS*) malloc(sizeof(RDS_MODULE_ENTRY_POINTS));
	CopyMemory(connector->pEntryPoints, &EntryPoints, sizeof(RDS_MODULE_ENTRY_POINTS));

	connector->GetEventHandles = xrdp_client_get_event_handles;
	connector->CheckEventHandles = xrdp_client_check_event_handles;

	connector->client = freerds_client_outbound_interface_new();
	connector->server = freerds_server_outbound_interface_new();

	connector->OutboundStream = Stream_New(NULL, 8192);
	connector->InboundStream = Stream_New(NULL, 8192);

	connector->InboundTotalLength = 0;
	connector->InboundTotalCount = 0;

	connector->OutboundTotalLength = 0;
	connector->OutboundTotalCount = 0;

	connector->StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	connector->pEntryPoints->New((rdsModuleConnector*) connector);

	connector->ServerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) xrdp_client_thread,
			(void*) connector, CREATE_SUSPENDED, NULL);

	freerds_client_inbound_connector_init(connector);

	connector->pEntryPoints->Start(connector);

	ResumeThread(connector->ServerThread);

	return connector;
}

void xrdp_module_free(rdsModuleConnector* module)
{
	rdsModuleConnector* connector;

	connector = (rdsModuleConnector*) module;

	SetEvent(connector->StopEvent);

	WaitForSingleObject(connector->ServerThread, INFINITE);
	CloseHandle(connector->ServerThread);

	Stream_Free(module->OutboundStream, TRUE);
	Stream_Free(module->InboundStream, TRUE);

	CloseHandle(connector->StopEvent);
	CloseHandle(module->hClientPipe);

	connector->pEntryPoints->Free(module);

	free(module);
}
