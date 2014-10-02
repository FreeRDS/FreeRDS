/**
 * Property C Wrapper is a way so the C Modules can
 * get access to the property manager
 *
 * there will be 3 kinds of propertys
 * PropertyBool
 * PropertyString
 * PropertyNumber
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

#include "PropertyCWrapper.h"

#include <appcontext/ApplicationContext.h>
#include <string>

BOOL getPropertyBool(char* path, BOOL* value)
{
	return APP_CONTEXT.getPropertyManager()->getPropertyBool(std::string(path), value);
}

BOOL getPropertyNumber(char* path, long* value)
{
	return APP_CONTEXT.getPropertyManager()->getPropertyNumber(std::string(path), value);
}

BOOL getPropertyString(char* path, char* value, unsigned int valueLength)
{
	std::string stdvalue;
	BOOL retValue = APP_CONTEXT.getPropertyManager()->getPropertyString(std::string(path), stdvalue);

	if (!retValue)
		return FALSE;

	if (stdvalue.size() + 1 > valueLength)
	{
		return FALSE;
	}
	else
	{
		CopyMemory(value,stdvalue.c_str(), stdvalue.size()+1);
		return TRUE;
	}
}

int setPropertyBool(char* path, BOOL value)
{
	return 0;
}

int setPropertyNumber(char* path, long value)
{
	return 0;
}

int setPropertyString(char* path, char* value)
{
	return 0;
}
