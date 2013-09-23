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

xrdpModule* xrdp_module_new(xrdpSession* session)
{
	int error_code;
	int auth_status;
	xrdpModule* mod;
	rdpSettings* settings;

	settings = session->settings;

	auth_status = xrdp_authenticate(settings->Username, settings->Password, &error_code);

	mod = (xrdpModule*) malloc(sizeof(xrdpModule));
	ZeroMemory(mod, sizeof(xrdpModule));

	mod->size = sizeof(xrdpModule);
	mod->session = session;
	mod->settings = session->settings;
	mod->SessionId = 10;

	xrdp_client_module_init(mod);
	xrdp_server_module_init(mod);

	mod->client->Start(mod);

	if (mod->client->Connect(mod) != 0)
		return NULL;

	return mod;
}

void xrdp_module_free(xrdpModule* mod)
{
	free(mod);
}
