/*
 * modules.h
 *
 *  Created on: Oct 21, 2013
 *      Author: retro
 */

#ifndef MODULES_H_
#define MODULES_H_

#include "../common/config/PropertyCWrapper.h"

typedef struct rds_module_entry_points_v1 RDS_MODULE_ENTRY_POINTS_V1;
typedef RDS_MODULE_ENTRY_POINTS_V1 RDS_MODULE_ENTRY_POINTS;

struct _RDS_MODULE_COMMON
{
	WORD sessionId;
	char * authToken;
	char * userName;
	HANDLE userToken;
	char ** envBlock;
};
typedef struct _RDS_MODULE_COMMON RDS_MODULE_COMMON;


/**
 * Module Entry Points
 */

typedef RDS_MODULE_COMMON * (*pRdsModuleNew)();
typedef void (*pRdsModuleFree)(RDS_MODULE_COMMON * module);

typedef char * (*pRdsModuleStart)(RDS_MODULE_COMMON * module);
typedef int (*pRdsModuleStop)(RDS_MODULE_COMMON * module);

struct rds_module_entry_points_v1
{
	DWORD Version;

	pRdsModuleNew New;
	pRdsModuleFree Free;

	pRdsModuleStart Start;
	pRdsModuleStop Stop;
	char * ModuleName;

	pgetPropertyBool getPropertyBool;
	pgetPropertyNumber getPropertyNumber;
	pgetPropertyString getPropertyString;
};

#define RDS_MODULE_INTERFACE_VERSION	1
#define RDS_MODULE_ENTRY_POINT_NAME	"RdsModuleEntry"

typedef int (*pRdsModuleEntry)(RDS_MODULE_ENTRY_POINTS* pEntryPoints);



#endif /* MODULES_H_ */
