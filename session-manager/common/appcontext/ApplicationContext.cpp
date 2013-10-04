#include "ApplicationContext.h"


namespace freerds{
	namespace sessionmanager{


	ApplicationContext::ApplicationContext(){

		wLogLayout* layout;
		wLogAppender* appender;

		WLog_Init();

		mWLogRoot = WLog_GetRoot();

		WLog_SetLogAppenderType(mWLogRoot, WLOG_APPENDER_CONSOLE);

		appender = WLog_GetLogAppender(mWLogRoot);
		WLog_ConsoleAppender_SetOutputStream(mWLogRoot, (wLogConsoleAppender*) appender, WLOG_CONSOLE_STDERR);

		layout = WLog_GetLogLayout(mWLogRoot);
		WLog_Layout_SetPrefixFormat(mWLogRoot, layout, "[%lv:%mn] [%fl|%fn|%ln] - ");

		WLog_OpenAppender(mWLogRoot);


	}


	ApplicationContext::~ApplicationContext(){

		WLog_CloseAppender(mWLogRoot);

		WLog_Uninit();


	}

	sessionNS::SessionStore * ApplicationContext::getSessionStore(){
		return &mSessionStore;
	}

	int ApplicationContext::startRPCEngine(){
		return mRpcEngine.startEngine();
	}

	int ApplicationContext::stopRPCEngine(){
		return mRpcEngine.stopEngine();
	}


    }
}
