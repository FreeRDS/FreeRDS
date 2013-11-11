/**
 * Application Context of Session Manager
 *
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thinstuff.at>
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
#include <winpr/library.h>
#include <winpr/environment.h>

#include <vector>
#include <utils/StringHelpers.h>

#include "ApplicationContext.h"

namespace freerds
{
	namespace sessionmanager
	{
		ApplicationContext::ApplicationContext()
		{
			wLogLayout* layout;
			wLogAppender* appender;

			initPaths();
			configureExecutableSearchPath();

			WLog_Init();

			mWLogRoot = WLog_GetRoot();

			WLog_SetLogAppenderType(mWLogRoot, WLOG_APPENDER_CONSOLE);

			appender = WLog_GetLogAppender(mWLogRoot);
			WLog_ConsoleAppender_SetOutputStream(mWLogRoot, (wLogConsoleAppender*) appender, WLOG_CONSOLE_STDERR);

			layout = WLog_GetLogLayout(mWLogRoot);
			WLog_Layout_SetPrefixFormat(mWLogRoot, layout, "[%lv:%mn] [%fl|%fn|%ln] - ");

			WLog_OpenAppender(mWLogRoot);
			WLog_SetLogLevel(mWLogRoot, WLOG_TRACE);

			setupTestingPropValues();
		}

		ApplicationContext::~ApplicationContext()
		{
			WLog_CloseAppender(mWLogRoot);
			WLog_Uninit();

			uninitPaths();
		}

		char* ApplicationContext::getHomePath()
		{
			if (!this->homePath)
			{
				char* p;
				int length;
				char separator;
				char moduleFileName[4096];

				separator = PathGetSeparatorA(PATH_STYLE_NATIVE);
				GetModuleFileNameA(NULL, moduleFileName, sizeof(moduleFileName));

				p = strrchr(moduleFileName, separator);
				*p = '\0';
				p = strrchr(moduleFileName, separator);
				*p = '\0';

				length = strlen(moduleFileName);
				this->homePath = (char*) malloc(length + 1);
				CopyMemory(this->homePath, moduleFileName, length);
				this->homePath[length] = '\0';
			}

			return this->homePath;
		}

		char* ApplicationContext::getLibraryPath()
		{
			if (!this->libraryPath)
			{
				getHomePath();
				this->libraryPath = GetCombinedPath(this->homePath, FREERDS_INSTALL_LIBDIR);
			}

			return this->libraryPath;
		}

		char* ApplicationContext::getExecutablePath()
		{
			if (!this->executablePath)
			{
				getHomePath();
				this->executablePath = GetCombinedPath(this->homePath, FREERDS_INSTALL_BINDIR);
			}

			return this->executablePath;
		}

		char* ApplicationContext::getShareDataPath()
		{
			if (!this->shareDataPath)
			{
				char* rootShareDataPath;

				getHomePath();

				rootShareDataPath = GetCombinedPath(this->homePath, FREERDS_INSTALL_DATAROOTDIR);
				this->shareDataPath = GetCombinedPath(rootShareDataPath, "freerds");
				free(rootShareDataPath);
			}

			return this->shareDataPath;
		}

		char* ApplicationContext::getSystemConfigPath()
		{
			if (!this->systemConfigPath)
			{
				char* rootSystemConfigPath;

				getHomePath();

				rootSystemConfigPath = GetCombinedPath(this->homePath, FREERDS_INSTALL_SYSCONFDIR);
				this->systemConfigPath = GetCombinedPath(rootSystemConfigPath, "freerds");
				free(rootSystemConfigPath);
			}

			return this->systemConfigPath;
		}

		void ApplicationContext::initPaths()
		{
			getHomePath();
			getLibraryPath();
			getExecutablePath();
			getShareDataPath();
			getSystemConfigPath();
		}

		void ApplicationContext::uninitPaths()
		{
			free(this->homePath);
			free(this->libraryPath);
			free(this->executablePath);
			free(this->shareDataPath);
			free(this->systemConfigPath);
		}

		void ApplicationContext::configureExecutableSearchPath()
		{
			int index;
			DWORD nSize;

			initPaths();

			nSize = GetEnvironmentVariableA("PATH", NULL, 0);

			if (nSize)
			{
				std::string pathExtra = "";
				bool executablePathPresent = false;
				bool systemConfigPathPresent = false;

				char* lpBuffer = (LPSTR) malloc(nSize);
				nSize = GetEnvironmentVariableA("PATH", lpBuffer, nSize);

				std::string pathEnv(lpBuffer);
				std::vector<std::string> pathList = split<std::string>(pathEnv, ":");

				for (index = 0; index < pathList.size(); index++)
				{
					if (strcmp(this->executablePath, pathList[index].c_str()) == 0)
						executablePathPresent = true;
					else if (strcmp(this->systemConfigPath, pathList[index].c_str()) == 0)
						systemConfigPathPresent = true;
				}

				if (!executablePathPresent)
				{
					pathExtra += this->executablePath;
					pathExtra += ":";
				}

				if (!systemConfigPathPresent)
				{
					pathExtra += this->systemConfigPath;
					pathExtra += ":";
				}

				if (pathExtra.length() > 0)
				{
					pathEnv = pathExtra + pathEnv;
					SetEnvironmentVariableA("PATH", pathEnv.c_str());
				}
			}
		}

		sessionNS::SessionStore* ApplicationContext::getSessionStore()
		{
			return &mSessionStore;
		}

		configNS::PropertyManager* ApplicationContext::getPropertyManager()
		{
			return &mPropertyManager;
		}

		int ApplicationContext::startRPCEngine()
		{
			return mRpcEngine.startEngine();
		}

		int ApplicationContext::stopRPCEngine()
		{
			return mRpcEngine.stopEngine();
		}

		SignalingQueue<callNS::Call> * ApplicationContext::getRpcOutgoingQueue()
		{
			return &mRpcOutgoingCalls;
		}

		int ApplicationContext::loadModulesFromPath(std::string path)
		{
			LPCSTR pszExt;
			char pattern[256];

			pszExt = PathGetSharedLibraryExtensionA(0);
			sprintf_s(pattern, 256, "*freerds-module-*.%s", pszExt);

			return mModuleManager.loadModulesFromPathAndEnv(path, pattern);
		}

		moduleNS::ModuleManager* ApplicationContext::getModuleManager()
		{
			return &mModuleManager;
		}

		void ApplicationContext::setupTestingPropValues()
		{
			mPropertyManager.setPropertyNumber(Global, 0, "module.x11.xres",1024);
			mPropertyManager.setPropertyNumber(Global, 0, "module.x11.yres",768);
			mPropertyManager.setPropertyNumber(Global, 0, "module.x11.colordepth",24);
		}
	}
}
