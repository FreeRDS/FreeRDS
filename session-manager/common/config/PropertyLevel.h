/**
 * Property levels
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

#ifndef PROPERTYLEVEL_H_
#define PROPERTYLEVEL_H_

typedef enum _PROPERTY_LEVEL
{
	Global = 1,
	UserGroup = 2,
	User = 3
}
PROPERTY_LEVEL, *PPROPERTY_LEVEL;

#endif /* PROPERTYLEVEL_H_ */
