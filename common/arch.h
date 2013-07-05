/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
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

#if !defined(ARCH_H)
#define ARCH_H

#include <winpr/wtypes.h>

#if defined(_WIN64)
/* Microsoft's VC++ compiler uses the more backwards-compatible LLP64 model.
   Most other 64 bit compilers(Solaris, AIX, HP, Linux, Mac OS X) use
   the LP64 model.
   long is 32 bits in LLP64 model, 64 bits in LP64 model. */
typedef __int64 tbus;
#else
typedef long tbus;
#endif

#endif
