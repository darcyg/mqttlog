/* ****************************************************************************************************
 * lib_os_basic.c within the following project: bld_device_cmake_LINUX
 *	
 *  compiler:   GNU Tools ARM Embedded (4.7.201xqx)
 *  target:     Cortex Mx
 *  author:		thomas
 * ****************************************************************************************************/

/* ****************************************************************************************************/

/*
 *	******************************* change log *******************************
 *  date			user			comment
 * 	Apr 3, 2018			thomas			- creation of lib_os_basic.c
 *  
 */



/* *******************************************************************
 * includes
 * ******************************************************************/
#include "lib_os_basic.h"

/* *******************************************************************
 * static function declarations
 * ******************************************************************/
lib_version_t lib_ob_basic__get_version(void){
	lib_version_t version;
	version.major =0;
	version.minor =1;
	version.build =1;
	return version;
}
