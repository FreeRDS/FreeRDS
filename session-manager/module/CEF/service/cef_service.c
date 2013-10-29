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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>
#include <winpr/cmdline.h>

#include "cef_service.h"

COMMAND_LINE_ARGUMENT_A rds_service_args[] =
{
	{ "session-id", COMMAND_LINE_VALUE_REQUIRED, "<session id>", NULL, NULL, -1, NULL, "session id" },
	{ NULL, 0, NULL, NULL, NULL, -1, NULL, NULL }
};

void rds_parse_arguments(rdsContextCef* rds, int argc, char** argv)
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
			rds->SessionId = atoi(arg->Value);
		}

		CommandLineSwitchEnd(arg)
	}
	while ((arg = CommandLineFindNextArgumentA(arg)) != NULL);
}

int main(int argc, char** argv)
{
	DWORD dwExitCode = 0;



	return dwExitCode;
}
