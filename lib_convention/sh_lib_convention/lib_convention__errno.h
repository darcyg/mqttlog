/* ****************************************************************************************************
 * lib_convention__errno.h within the following project: lib_convention
 *	
 *  compiler:   GNU Tools ARM Embedded (4.7.201xqx)
 *  target:     Cortex Mx
 *  author:		schmied
 * ****************************************************************************************************/

/* ****************************************************************************************************/

/*
 *	******************************* change log *******************************
 *  date			user			comment
 * 	2 Dec 2014			schmied			- creation of lib_convention__errno.h
 *  
 */



#ifndef GEN_LIB_CONVENTION__ERRNO_H_
#define GEN_LIB_CONVENTION__ERRNO_H_

/* *******************************************************************
 * includes
 * ******************************************************************/

/* *******************************************************************
 * defines
 * ******************************************************************/
#ifndef NULL
  #define NULL      0
#endif

/* *******************************************************************
 * >>>>>	unhandy fault										<<<<<<
 * ******************************************************************/
#define EHAL_ERROR		1006
#define EEXEC_NOINIT    1005
#define ECONFIG_INVALID 1004
#define ETIME_OUT       1003
#define ENOT_SUPP		1002 	/* destroys the world */
#define EPAR_NULL		1001	/* NULL pointer argument */
#define EAGAIN_INIT		1000    /* device already initialized */
#define EOK				0

/* *******************************************************************
 * custom data types (e.g. enumerations, structures, unions)
 * ******************************************************************/


/* *******************************************************************
 * (static) variables declarations
 * ******************************************************************/


/* *******************************************************************
 * static function declarations
 * ******************************************************************/

#endif /* GEN_LIB_CONVENTION__ERRNO_H_ */
