#include "config.h"
#include <iostream>

#include <appcontext/ApplicationContext.h>

using namespace std;
int main(void)
{
	std::string test;
	APP_CONTEXT.startRPCEngine();
	cout << "Hello session manager" << endl;
	cin >>test;
	APP_CONTEXT.stopRPCEngine();
	return 0;
}
