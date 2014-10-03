/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * xrdp-ng interprocess communication protocol
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
#include <winpr/synch.h>
#include <winpr/environment.h>

#include <freerds/backend.h>

BOOL combinePaths(char* buffer, int buffersize, char* basePath, char* prop)
{
	int basePathLength = strlen(basePath);
	int propLength = strlen(prop);
	int fullLength = basePathLength + propLength + 2;

	if (fullLength > buffersize)
		return FALSE;

	CopyMemory(buffer,basePath,basePathLength);
	buffer[basePathLength] = '.';
	CopyMemory(buffer + basePathLength + 1, prop, propLength);
	buffer[fullLength - 1] = 0;

	return TRUE;
}

BOOL getPropertyNumberWrapper(char* basePath, RDS_MODULE_CONFIG_CALLBACKS* config, char* path, long* value)
{
	char tempBuffer[1024];

	if (!combinePaths(tempBuffer, 1024, basePath, path))
		return FALSE;

	return config->getPropertyNumber(tempBuffer, value);
}

BOOL getPropertyStringWrapper(char* basePath, RDS_MODULE_CONFIG_CALLBACKS* config, char* path, char* value, unsigned int valueLength)
{
	char tempBuffer[1024];

	if (!combinePaths(tempBuffer, 1024, basePath, path))
		return FALSE;

	return config->getPropertyString(tempBuffer, value, valueLength);
}

void initResolutions(char* basePath, RDS_MODULE_CONFIG_CALLBACKS* config, char** envBlock, long* xres, long* yres, long* colordepth)
{
	char tempstr[256];
	long maxXRes = 0 , maxYRes = 0, minXRes = 0, minYRes = 0;
	long connectionXRes = 0, connectionYRes = 0, connectionColorDepth = 0;

	wLog* logger_module_helper = WLog_Get("freerds.sessionmanager.module.helper");

	if (!getPropertyNumberWrapper(basePath, config, "maxXRes", &maxXRes)) {
		WLog_Print(logger_module_helper, WLOG_ERROR, "Setting: %s.maxXRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n",basePath);
	}
	if (!getPropertyNumberWrapper(basePath, config, "maxYRes", &maxYRes)) {
		WLog_Print(logger_module_helper, WLOG_ERROR, "Setting: %s.maxYRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n",basePath);
	}
	if (!getPropertyNumberWrapper(basePath, config, "minXRes", &minXRes)) {
		WLog_Print(logger_module_helper, WLOG_ERROR, "Setting: %s.minXRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n",basePath);
	}
	if (!getPropertyNumberWrapper(basePath, config, "minYRes", &minYRes)){
		WLog_Print(logger_module_helper, WLOG_ERROR, "Setting: %s.minYRes not defined, NOT setting FREERDS_SMAX or FREERDS_SMIN\n",basePath);
	}

	if ((maxXRes != 0) && (maxYRes != 0)){
		sprintf_s(tempstr, sizeof(tempstr), "%dx%d", (unsigned int) maxXRes, (unsigned int) maxYRes );
		SetEnvironmentVariableEBA(envBlock, "FREERDS_SMAX", tempstr);
	}
	if ((minXRes != 0) && (minYRes != 0)) {
		sprintf_s(tempstr, sizeof(tempstr), "%dx%d", (unsigned int) minXRes, (unsigned int) minYRes );
		SetEnvironmentVariableEBA(envBlock, "FREERDS_SMIN", tempstr);
	}

	config->getPropertyNumber("current.connection.xres", &connectionXRes);
	config->getPropertyNumber("current.connection.yres", &connectionYRes);
	config->getPropertyNumber("current.connection.colordepth", &connectionColorDepth);

	if ((connectionXRes == 0) || (connectionYRes == 0))
	{
		WLog_Print(logger_module_helper, WLOG_ERROR, "got no XRes or YRes from client, using config values");

		if (!getPropertyNumberWrapper(basePath, config, "xres", xres))
			*xres = 1024;

		if (!getPropertyNumberWrapper(basePath, config, "yres", yres))
			*yres = 768;

		if (!getPropertyNumberWrapper(basePath, config, "colordepth", colordepth))
			*colordepth = 24;

		return;
	}

	if ((maxXRes > 0 ) && (connectionXRes > maxXRes)) {
		*xres = maxXRes;
	} else if ((minXRes > 0 ) && (connectionXRes < minXRes)) {
		*xres = minXRes;
	} else {
		*xres = connectionXRes;
	}

	if ((maxYRes > 0 ) && (connectionYRes > maxYRes)) {
		*yres = maxYRes;
	} else if ((minYRes > 0 ) && (connectionYRes < minYRes)) {
		*yres = minYRes;
	} else {
		*yres = connectionYRes;
	}

	if (connectionColorDepth == 0) {
		connectionColorDepth = 16;
	}

	*colordepth = connectionColorDepth;
}
