/* ****************************************************************************************************
 * lib_thread_test.c within the following project: lib_thread
 *
 *  compiler:   GNU Tools ARM LINUX
 *  target:     armv6
 *  author:	    Tom
 * ****************************************************************************************************/

/* ****************************************************************************************************/

/*
 *	******************************* change log *******************************
 *  date			user			comment
 * 	16.12.2017			Tom			- creation of lib_thread.c
 *
 */


// ////////////////////////////////////////////////////////
// Includes
// ////////////////////////////////////////////////////////

/* c-runtime */
#include <stdio.h>
#include <stdlib.h>

/* system */

/* own libs */
#include <embUnit.h>
/* project */
#include "lib_thread_test.h"


//		new_TestFixture("test_lib_can__mscp_client_cleanup", test_lib_can__mscp_client_cleanup),
/* *************************************************************************************************
 * \brief			Main
 * \description		Creates and executes the test runner instance
 * \return			int
 * *************************************************************************************************/
int main() {
    TestRunner_start();
    TestRunner_runTest(lib_thread_test());
    TestRunner_end();
	return 0;
}
