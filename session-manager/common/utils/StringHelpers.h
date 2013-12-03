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

#ifndef __STRING_HELPERS_HEADER_H
#define __STRING_HELPERS_HEADER_H

#include <vector>
#include <string>

template<typename T>
std::vector<T>
split(const T & str, const T & delimiters) {
	std::vector<T> v;
    typename T::size_type start = 0;
    typename T::size_type pos = str.find_first_of(delimiters, start);
    while (pos != T::npos) {
        if(pos != start) // ignore empty tokens
        	v.push_back(str.substr(start, pos - start));
        start = pos + 1;
        pos = str.find_first_of(delimiters, start);
    }
    if(start < str.length()) // ignore trailing delimiter
    	v.push_back(str.substr(start,  str.length() - start));

    return v;
}

namespace std{

	bool stringEndsWith(const string& compString, const string& suffix);
}

#endif
