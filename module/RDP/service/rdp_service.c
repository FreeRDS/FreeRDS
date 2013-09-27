/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * RDP Module Service
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>
#include <winpr/cmdline.h>

#include <freerdp/freerdp.h>
#include <freerdp/settings.h>

#include <freerdp/client/file.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/client/channels.h>

#include "rdp_client.h"

#include "rdp_service.h"

COMMAND_LINE_ARGUMENT_A rds_service_args[] =
{
	{ "session-id", COMMAND_LINE_VALUE_REQUIRED, "<session id>", NULL, NULL, -1, NULL, "session id" },
	{ NULL, 0, NULL, NULL, NULL, -1, NULL, NULL }
};

void rds_parse_arguments(rdpContext* context, int argc, char** argv)
{
	int status;
	DWORD flags;
	COMMAND_LINE_ARGUMENT_A* arg;

	flags = COMMAND_LINE_SEPARATOR_COLON;
	flags |= COMMAND_LINE_SIGIL_SLASH | COMMAND_LINE_SIGIL_PLUS_MINUS;

	status = CommandLineParseArgumentsA(argc, (const char**) argv,
			rds_service_args, flags, NULL, NULL, NULL);

	arg = rds_service_args;

	do
	{
		if (!(arg->Flags & COMMAND_LINE_VALUE_PRESENT))
			continue;

		CommandLineSwitchStart(arg)

		CommandLineSwitchCase(arg, "session-id")
		{

		}

		CommandLineSwitchEnd(arg)
	}
	while ((arg = CommandLineFindNextArgumentA(arg)) != NULL);
}

int main(int argc, char** argv)
{
	int status;
	HANDLE thread;
	DWORD dwExitCode;
	rdpContext* context;
	rdpSettings* settings;
	RDP_CLIENT_ENTRY_POINTS clientEntryPoints;

	ZeroMemory(&clientEntryPoints, sizeof(RDP_CLIENT_ENTRY_POINTS));
	clientEntryPoints.Size = sizeof(RDP_CLIENT_ENTRY_POINTS);
	clientEntryPoints.Version = RDP_CLIENT_INTERFACE_VERSION;

	RDS_RdpClientEntry(&clientEntryPoints);

	context = freerdp_client_context_new(&clientEntryPoints);

	settings = context->settings;

	freerdp_register_addin_provider(freerdp_channels_load_static_addin_entry, 0);

	status = freerdp_client_parse_command_line(context, argc, argv);

	status = freerdp_client_command_line_status_print(argc, argv, settings, status);

	if (status)
	{
		freerdp_client_context_free(context);
		return 0;
	}

	rds_parse_arguments(context, argc, argv);

	freerdp_client_start(context);

	thread = freerdp_client_get_thread(context);

	WaitForSingleObject(thread, INFINITE);

	GetExitCodeThread(thread, &dwExitCode);

	freerdp_client_stop(context);

	freerdp_client_context_free(context);

	return dwExitCode;
}
