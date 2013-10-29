/**
 * FreeRDS module connector interface
 * module connector provides the glue between a service module and rds
 *
 * Copyright 2013 Thinstuff Technologies GmbH
 * Copyright 2013 Bernhard Miklautz <bmiklautz@thinstuff.at>
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
#ifndef MODULE_CONNECTOR_H_
#define MODULE_CONNECTOR_H_

#include <freerds/freerds.h>

#ifdef __cplusplus
extern "C" {
#endif

rdsModuleConnector* freerds_module_connector_new(rdsConnection* connection);
void freerds_module_connector_free(rdsModuleConnector* connector);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_CONNECTOR_H_ */
