 /**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng authentication
 *
 * Copyright (C) Jay Sorg 2004-2012
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
#include <winpr/path.h>

#include <security/pam_appl.h>

#include "freerds.h"

struct t_user_pass
{
	char user[256];
	char pass[256];
};

struct t_auth_info
{
	struct t_user_pass user_pass;
	int session_opened;
	int did_setcred;
	struct pam_conv pamc;
	pam_handle_t *ph;
};

static void get_service_name(char* service_name)
{
	service_name[0] = 0;

	if (PathFileExistsA("/etc/pam.d/freerds"))
	{
		strncpy(service_name, "freerds", 255);
	}
	else
	{
		strncpy(service_name, "gdm", 255);
	}
}

static int verify_pam_conv(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
{
	int i;
	struct pam_response* reply;
	struct t_user_pass* user_pass;

	reply = malloc(sizeof(struct pam_response) * num_msg);

	if (!reply)
		return -1;

	ZeroMemory(reply, sizeof(struct pam_response) * num_msg);

	for (i = 0; i < num_msg; i++)
	{
		switch (msg[i]->msg_style)
		{
			case PAM_PROMPT_ECHO_ON: /* username */
				user_pass = appdata_ptr;
				reply[i].resp = _strdup(user_pass->user);
				reply[i].resp_retcode = PAM_SUCCESS;
				break;

			case PAM_PROMPT_ECHO_OFF: /* password */
				user_pass = appdata_ptr;
				reply[i].resp = _strdup(user_pass->pass);
				reply[i].resp_retcode = PAM_SUCCESS;
				break;

			default:
				printf("unknown in verify_pam_conv\r\n");
				free(reply);
				return PAM_CONV_ERR;
		}
	}

	*resp = reply;
	return PAM_SUCCESS;
}

long freerds_authenticate(char* username, char* password, int* errorcode)
{
	int error;
	char service_name[256];
	struct t_auth_info* auth_info;

	get_service_name(service_name);
	auth_info = malloc(sizeof(struct t_auth_info));
	ZeroMemory(auth_info, sizeof(struct t_auth_info));
	strcpy(auth_info->user_pass.user, username);
	strcpy(auth_info->user_pass.pass, password);
	auth_info->pamc.conv = &verify_pam_conv;
	auth_info->pamc.appdata_ptr = &(auth_info->user_pass);
	error = pam_start(service_name, 0, &(auth_info->pamc), &(auth_info->ph));

	if (error != PAM_SUCCESS)
	{
		if (errorcode != NULL)
			*errorcode = error;

		printf("pam_start failed: %s\n", pam_strerror(auth_info->ph, error));
		free(auth_info);
		return 0;
	}

	error = pam_authenticate(auth_info->ph, 0);

	if (error != PAM_SUCCESS)
	{
		if (errorcode != NULL)
			*errorcode = error;

		printf("pam_authenticate failed: %s\n", pam_strerror(auth_info->ph, error));
		free(auth_info);
		return 0;
	}

	error = pam_acct_mgmt(auth_info->ph, 0);

	if (error != PAM_SUCCESS)
	{
		if (errorcode != NULL)
			*errorcode = error;

		printf("pam_acct_mgmt failed: %s\n", pam_strerror(auth_info->ph, error));
		free(auth_info);
		return 0;
	}

	return (long) auth_info;
}
