/* ****************************************************************************************************
 * lib_thread.c within the following project: lib_thread
 *
 *  compiler:   GNU Tools ARM LINUX
 *  target:     armv6
 *  author:	    Tom
 * ****************************************************************************************************/

/* ****************************************************************************************************/

/*
 *	******************************* change log *******************************
 *  date			user			comment
 * 	06 April 2015			Tom			- creation of lib_thread.c
 *  21 April 2015			Tom			- add of comments
 *
 */

#ifndef _VA_TEST_LIB_THREAD_LIB_THREAD_TEST_H_
#define _VA_TEST_LIB_THREAD_LIB_THREAD_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif


/* *******************************************************************
 * includes
 * ******************************************************************/

/* c-runtime */

/* system */

/* own libs */
#include <embUnit.h>

/* project */


/* *******************************************************************
 * Defines
 * ******************************************************************/

/* *******************************************************************
 * custom data types (e.g. enumerations, structures, unions)
 * ******************************************************************/

/* *******************************************************************
 * Global Functions
/* *******************************************************************/
TestRef lib_thread_test(void);

#ifdef __cplusplus
}
#endif

#endif /* _VA_TEST_LIB_THREAD_LIB_THREAD_TEST_H_ */
