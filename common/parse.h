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
 *
 * Parsing structs and macros
 *
 * based on parse.h from rdesktop
 * this is a super fast stream method, you bet
 * needed functions g_malloc, g_free, g_memset, g_memcpy
 */

#if !defined(PARSE_H)
#define PARSE_H

#include "arch.h"

#include <winpr/crt.h>
#include <winpr/stream.h>

#define s_check_rem_len(s, n) ((s)->pointer + (n) <= (s)->buffer + (s)->length)

#define make_stream(s) \
  (s) = (wStream*) g_malloc(sizeof(wStream), 1)

#define init_stream(s, v) do \
{ \
  if ((v) > (s)->capacity) \
  { \
    g_free((s)->buffer); \
    (s)->buffer = (BYTE*) g_malloc((v), 0); \
    (s)->capacity = (v); \
  } \
  (s)->pointer = (s)->buffer; \
  (s)->length = 0; \
} while (0)

#define free_stream(s) do \
{ \
  if ((s) != 0) \
  { \
    g_free((s)->buffer); \
  } \
  g_free((s)); \
} while (0)

#endif
