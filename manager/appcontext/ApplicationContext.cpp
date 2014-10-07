/**
 * Application Context of Session Manager
 *
 * Copyright 2013 Thincast Technologies GmbH
 * Copyright 2013 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
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

#include <list>
#include <utils/StringHelpers.h>

#include "ApplicationContext.h"

#include <session/TaskSessionTimeout.h>

namespace freerds
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

		setupTestingPropValues();
	}

	ApplicationContext::~ApplicationContext()
	{
		WLog_CloseAppender(mWLogRoot);
		WLog_Uninit();
	}

	std::string ApplicationContext::getHomePath()
	{
		if (mHomePath.size() == 0)
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

			mHomePath.assign(moduleFileName);
		}
		return mHomePath;
	}

	std::string ApplicationContext::getLibraryPath()
	{
		if (mLibraryPath.size() == 0)
		{
			mLibraryPath = getHomePath();
			mLibraryPath += "/";
			mLibraryPath += FREERDS_INSTALL_LIBDIR;
		}
		return mLibraryPath;
	}

	std::string ApplicationContext::getExecutablePath()
	{
		if (mExecutablePath.size() == 0)
		{
			mExecutablePath = getHomePath();
			mExecutablePath += "/";
			mExecutablePath += FREERDS_INSTALL_BINDIR;
		}
		return mExecutablePath;
	}

	std::string ApplicationContext::getShareDataPath()
	{
		if (mShareDataPath.size() == 0)
		{
			mShareDataPath = getHomePath();
			mShareDataPath += "/";
			mShareDataPath += FREERDS_INSTALL_DATAROOTDIR;
			mShareDataPath += "/";
			mShareDataPath += "freerds";
		}
		return mShareDataPath;
	}

	std::string ApplicationContext::getSystemConfigPath()
	{
		if (mSystemConfigPath.size() == 0)
		{

			mSystemConfigPath = getHomePath();
			mSystemConfigPath += "/";
			mSystemConfigPath += FREERDS_INSTALL_SYSCONFDIR;
			mSystemConfigPath += "/";
			mSystemConfigPath += "freerds";
		}
		return mSystemConfigPath;
	}

	void ApplicationContext::exportContext()
	{
		int size;
		FILE* fp;
		char iniStr[2048];

		const char iniFmt[] =
			"[FreeRDS]\n"
			"prefix=\"%s\"\n"
			"bindir=\"%s\"\n"
			"sbindir=\"%s\"\n"
			"libdir=\"%s\"\n"
			"datarootdir=\"%s\"\n"
			"localstatedir=\"%s\"\n"
			"sysconfdir=\"%s\"\n"
			"\n";

		sprintf_s(iniStr, sizeof(iniStr), iniFmt,
			mHomePath.c_str(), /* prefix */
			FREERDS_INSTALL_BINDIR, /* bindir */
			FREERDS_INSTALL_SBINDIR, /* sbindir */
			FREERDS_INSTALL_LIBDIR, /* libdir */
			FREERDS_INSTALL_DATAROOTDIR, /* datarootdir */
			FREERDS_INSTALL_LOCALSTATEDIR, /* localstatedir */
			FREERDS_INSTALL_SYSCONFDIR /* sysconfdir */
			);

		size = strlen(iniStr) + 1;

		fp = fopen("/var/run/freerds.instance", "w");

		if (fp)
		{
			fwrite(iniStr, 1, size, fp);
			fclose(fp);
		}
	}

	void ApplicationContext::initPaths()
	{
		getHomePath();
		getLibraryPath();
		getExecutablePath();
		getShareDataPath();
		getSystemConfigPath();

		exportContext();
	}

	void ApplicationContext::configureExecutableSearchPath()
	{
		int index;
		DWORD nSize;

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
				if (strcmp(mExecutablePath.c_str(), pathList[index].c_str()) == 0)
					executablePathPresent = true;
				else if (strcmp(mSystemConfigPath.c_str(), pathList[index].c_str()) == 0)
					systemConfigPathPresent = true;
			}

			if (!executablePathPresent)
			{
				pathExtra += mExecutablePath;
				pathExtra += ":";
			}

			if (!systemConfigPathPresent)
			{
				pathExtra += mSystemConfigPath;
				pathExtra += ":";
			}

			if (pathExtra.length() > 0)
			{
				pathEnv = pathExtra + pathEnv;
				SetEnvironmentVariableA("PATH", pathEnv.c_str());
			}
		}
	}

	SessionStore* ApplicationContext::getSessionStore()
	{
		return &mSessionStore;
	}

	ConnectionStore* ApplicationContext::getConnectionStore()
	{
		return &mConnectionStore;
	}

	PropertyManager* ApplicationContext::getPropertyManager()
	{
		return &mPropertyManager;
	}

	int ApplicationContext::startRPCEngine()
	{
		mFDSApiServer.startFDSApi();
		return mRpcEngine.startEngine();
	}

	int ApplicationContext::stopRPCEngine()
	{
		mFDSApiServer.stopFDSApi();
		return mRpcEngine.stopEngine();
	}

	SignalingQueue<Call*> * ApplicationContext::getRpcOutgoingQueue()
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

	ModuleManager* ApplicationContext::getModuleManager()
	{
		return &mModuleManager;
	}

	FDSApiServer* ApplicationContext::getFDSApiServer()
	{
		return &mFDSApiServer;
	}

	void ApplicationContext::setupTestingPropValues()
	{
		mPropertyManager.setPropertyString("module","xsession");
		mPropertyManager.setPropertyString("auth.module","PAM");
		mPropertyManager.setPropertyString("auth.greeter","greeter");
		mPropertyManager.setPropertyBool("session.reconnect",true);
		mPropertyManager.setPropertyNumber("session.timeout",60);

		mPropertyManager.setPropertyString("module.xsession.modulename","X11");
		mPropertyManager.setPropertyNumber("module.xsession.maxXRes",1920);
		mPropertyManager.setPropertyNumber("module.xsession.maxYRes",1200);
		mPropertyManager.setPropertyNumber("module.xsession.minXRes",320);
		mPropertyManager.setPropertyNumber("module.xsession.minYRes",200);
		mPropertyManager.setPropertyNumber("module.xsession.xres",1024);
		mPropertyManager.setPropertyNumber("module.xsession.yres",768);
		mPropertyManager.setPropertyNumber("module.xsession.colordepth",24);
		mPropertyManager.setPropertyString("module.xsession.startwm","startwm.sh");

		mPropertyManager.setPropertyString("module.greeter.modulename","X11");
		mPropertyManager.setPropertyNumber("module.greeter.maxXRes",1920);
		mPropertyManager.setPropertyNumber("module.greeter.maxYRes",1200);
		mPropertyManager.setPropertyNumber("module.greeter.minXRes",320);
		mPropertyManager.setPropertyNumber("module.greeter.minYRes",200);
		mPropertyManager.setPropertyNumber("module.greeter.xres",1024);
		mPropertyManager.setPropertyNumber("module.greeter.yres",768);
		mPropertyManager.setPropertyNumber("module.greeter.colordepth",24);
		mPropertyManager.setPropertyString("module.greeter.cmd","simple_greeter");
	}

	void ApplicationContext::startTaskExecutor() {
		mTaskExecutor.start();
	}

	void ApplicationContext::stopTaskExecutor() {
		mTaskExecutor.stop();
	}

	void ApplicationContext::startSessionTimoutMonitor(){
		TaskSessionTimeoutPtr task(new TaskSessionTimeout());
		addTask(task);
	}

	bool ApplicationContext::addTask(TaskPtr task) {
		return mTaskExecutor.addTask(task);
	}

	void ApplicationContext::rpcDisconnected()
	{
		getConnectionStore()->reset();
		std::list<SessionPtr> allSessions = getSessionStore()->getAllSessions();

		std::list<SessionPtr>::iterator iterator;

		for (iterator = allSessions.begin(); iterator != allSessions.end(); ++iterator)
		{
			SessionPtr currentSession = (*iterator);

			if (currentSession->isAuthSession())
			{
				currentSession->stopModule();
				getSessionStore()->removeSession(currentSession->getSessionID());
			}
			else
			{
				currentSession->setConnectState(WTSDisconnected);
			}
		}
	}
}

