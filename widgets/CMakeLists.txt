# FreeRDS: FreeRDP Remote Desktop Services (RDS)
#
# Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(QT4_FEATURE_TYPE "RECOMMENDED")
set(QT4_FEATURE_PURPOSE "Qt4")
set(QT4_FEATURE_DESCRIPTION "Qt4 library")
find_feature(Qt4 ${QT4_FEATURE_TYPE} ${QT4_FEATURE_PURPOSE} ${QT4_FEATURE_DESCRIPTION})

if(Qt4_FOUND)
	add_subdirectory(simple_greeter)
endif()

