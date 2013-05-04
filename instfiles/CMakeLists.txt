# FreeRDP X11 Server Next Generation
# xrdp-ng cmake build script
#
# Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

set(${MODULE_PREFIX}_CFG_DATA
	xrdp.sh
	km-0407.ini
	km-0409.ini
	km-040c.ini
	km-0410.ini
	km-0419.ini
	km-041d.ini
	km-0816.ini)

install(FILES ${${MODULE_PREFIX}_CFG_DATA} DESTINATION ${XRDP_CFG_PATH})


