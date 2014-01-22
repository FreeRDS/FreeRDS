/**
 * FreeRDS: FDSApi implementation for thrift
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

#ifndef FREERDS_FDSAPI_THRIFT_H
#define FREERDS_FDSAPI_THRIFT_H

#include <winpr/wtsapi.h>

#ifdef __cplusplus
extern "C" {
#endif

WINPR_API PWTSFunctionTable FDSApiEntry(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERDS_FDSAPI_THRIFT_H */
