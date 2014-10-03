/**
 * helper templates for strings
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

#include "StringHelpers.h"

namespace std
{
	bool stringEndsWith(const string& compString, const string& suffix)
	{
		return compString.rfind(suffix) == (compString.size()-suffix.size());
	}

	bool stringStartsWith(const string& string2comp, const string& startswith)
	{
	    return string2comp.size() >= startswith.size()
	        && equal(startswith.begin(), startswith.end(), string2comp.begin());
	}
}

namespace boost
{
	template<>
	bool lexical_cast<bool, std::string>(const std::string& arg)
	{
		std::istringstream ss(arg);
		bool b;
		ss >> std::boolalpha >> b;
		return b;
	}

	template<>
	std::string lexical_cast<std::string, bool>(const bool& b)
	{
		std::ostringstream ss;
		ss << std::boolalpha << b;
		return ss.str();
	}
}
