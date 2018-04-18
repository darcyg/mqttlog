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

/* *******************************************************************
 * includes
 * ******************************************************************/

/* c-runtime */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

/* system */
#include <dlfcn.h>
#include <unistd.h>

/* framework */
#include <lib_clock.h>
#include <embUnit.h>
#include <lib_log.h>
#include <lib_thread.h>
#include <lib_thread_wakeup.h>
#include <lib_convention__errno.h>


#ifdef __gnu_linux__
	#include <sched.h>		// POSIX scheduling parameter functions and definitions
	#include <pthread.h>
	#include <unistd.h>
	#include <fcntl.h>			// open()
	#include <sys/syscall.h>	// SYS_gettid
#endif

/* local libs */
#include "lib_thread_test.h"

/* *******************************************************************
 * Defines
 * ******************************************************************/
#define M_APP_LIB_THREAD_TEST_ID 		"APP_THD_TST"

#define ITER 900UL	// number of dummy cycle iterations to create CPU load

#define TASK_PRIO_PLUS4 	"test_4"
#define TASK_PRIO_PLUS3 	"test_3"
#define TASK_PRIO_PLUS2 	"test_2"
#define TASK_PRIO_PLUS1 	"test_1"
#define TASK_PRIO_MINUS1 	"test_-1"
#define TASK_PRIO_MINUS2 	"test_-2"
#define TASK_PRIO_MINUS3 	"test_-3"

#ifdef __gnu_linux__

#define PMINE	0
#define PMAXE	100
#define PCUR	10
#elif defined _WIN32
#define PMINE	-16
#define PMAXE	16
#define PCUR	0
#endif /*__gnu_linux__ or _WIN32 or __QNXNTO__ */



/* *******************************************************************
 * custom data types (e.g. enumerations, structures, unions)
 * ******************************************************************/
struct thread_test_attr {
	unsigned int running;
	uint32_t interval;
};



typedef int (*wakeup__init_t)(void);
typedef int (*wakeup__cleanup_t)(void);


struct test_lib_interfaces
{
	wakeup__init_t wakeup__init;
	wakeup__cleanup_t wakeup__cleanup;
};

struct signal_helper
{
	signal_hdl_t sgn;
	unsigned int signal_send_running;
};

struct condvar_wait_arg
{
	cond_hdl_t *cond;
	mutex_hdl_t *mtx;
	int			prioidx;
};


/* *******************************************************************
 * Static Variables
/* *******************************************************************/
static mutex_hdl_t mtx1= NULL, mtx2 = NULL, mtx3 = NULL, mtx4 = NULL;
static mutex_hdl_t *mtx_pair12[2], *mtx_pair23[2], *mtx_pair24[2];
static int s_wup[3] = {0, 0, 0};

static thread_hdl_t wakeup_worker_th;
static unsigned int wakeup_worker_running = 0;
static wakeup_hdl_t wakeup_obj;

static void *s_test_lib;
static struct test_lib_interfaces s_test_lib_interfaces;

static unsigned int temp_test_var = 0;

/* *******************************************************************
 * Static Function Prototypes
 * ******************************************************************/
static void* thread__test(void *_arg);
static void* join__test(void *_tid);
static void* thread__dummy(void *_arg);
static void* print__dummy(void *_dummy);
static void* priority__test(void *_mutex);
static void* mutex__test(void *_mutex);
static void* mutex__test_1(void *_mutex);
static void* mutex__test_2a(void *_mutex_pair);
static void* mutex__test_2b(void *_mutex_pair);
static void* mutex__test_2c(void *_mutex_pair);
static void* mutex__test_2d(void *_mutex_pair);
static void* mutex__test_2e(void *_mutex_pair);
static void* mutex__test_2f(void *_mutex_pair);
static void* mutex__test_2g(void *_mutex_pair);
static void* mutex__unlock_test(void *_mutex);
static void* signal_send(void *_arg);
static void* signal_wait(void *_arg);
static void* signal_timedwait(void *_arg);
static void* signal_destroy(void *_arg);
static void* condvar_wait(void *_arg);
static void* condvar_thread_lockandsignal_sleep(void *_arg);
static void* condvar_thread_waitandtimedwait(void *_arg);
static void* semaphore_test(void *_arg);

static void* test_lib_thread__wakeup (void *_arg);
static void* test_lib_thread__worker (void *_arg);


/* *******************************************************************
 * Static Functions
 * ******************************************************************/
static void setUp(void) {

}

static void tearDown(void) {

}

static void test_lib_thread__init(void)
{
	msg(LOG_LEVEL_error, M_APP_LIB_THREAD_TEST_ID, "Hello");
	int ret;
	ret = lib_thread__init(PROCESS_SCHED_rr,PCUR);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


static void test_lib_thread__create(void)
{
	int ret;
	thread_hdl_t	tid1 , tid2;
	char name[16] = {0};

	/* Bad Cases */
	ret = lib_thread__create(NULL,	&thread__test,	NULL,	1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__create(&tid1,	NULL, NULL,	1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__create(&tid1,	&thread__test,	NULL,	PMAXE-PCUR,	TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(-EPAR_RANGE, ret);

	ret = lib_thread__create(&tid1,	&thread__test,	NULL,	PMINE-PCUR,	TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(-EPAR_RANGE, ret);

	ret = lib_thread__create(&tid1,	&thread__dummy,	&tid1,	-1,	TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(40);		// give just created thread some time to execute
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);	// wait for dummy thread termination to get an invalid thread object afterwards
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	/* Good Cases */
	ret = lib_thread__create(&tid1, &thread__test, NULL, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__getname(tid1, name, sizeof(name));
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = strcmp(name, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &thread__test, NULL, 1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__getname(tid1, name, sizeof(name));
	TEST_ASSERT_EQUAL_INT(-ESTD_NOENT, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &thread__test, NULL, 1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid2, &join__test, (void*)&tid1, 1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

}

static void test_lib_thread__join(void)
{
	int ret;
	thread_hdl_t	tid1;
	struct thread_test_attr test_info = {.running = 0, .interval = 100};

	ret = lib_thread__create(&tid1,	&thread__test,	&test_info,	-1,	TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	while (test_info.running == 0) {
		lib_thread__msleep(test_info.interval);
	}
	test_info.running = 0;

	ret = lib_thread__join(NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__join(&tid1, NULL);	// wait for dummy thread termination to get an invalid thread object afterwards
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(-ESTD_SRCH, ret);

	ret = lib_thread__cancel(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cancel(tid1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);
}

static void test_lib_thread__getname(void)
{
	thread_hdl_t tid1 = NULL;
	struct thread_test_attr test_info = {.running = 0, .interval = 100};
	char name[16] = {0};
	int ret;

	ret = lib_thread__getname(tid1, NULL, sizeof(name));
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__getname(NULL, name, sizeof(name));
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__getname(tid1, name, 1	);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__create(&tid1,	&thread__test,	&test_info,	-1,	TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__getname(tid1, name, sizeof(name));
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = strcmp(name,TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	while (test_info.running == 0) {
		lib_thread__msleep(test_info.interval);
	}
	test_info.running = 0;

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

}

static void test_lib_thread__mutex(void)
{
	thread_hdl_t tid1 = NULL, tid2=NULL, tid3=NULL, tid4=NULL, tid5=NULL, tid6=NULL, tid7=NULL;
	mutex_hdl_t test_mtx = NULL;
	int ret;

	/*Bad Cases */

	ret = lib_thread__mutex_init(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_destroy(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_lock(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_trylock(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_unlock(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_init(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
//
	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);

	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__unlock_test, &mtx1, +1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_BUSY, ret);

	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(-EEXEC_DEADLK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_BUSY, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_INVAL, ret);

	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	/* Good Cases */

	ret = lib_thread__mutex_init(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx3);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx4);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	mtx_pair12[0] = &mtx1;	mtx_pair12[1] = &mtx2;
	mtx_pair23[0] = &mtx2;	mtx_pair23[1] = &mtx3;
	mtx_pair24[0] = &mtx2;	mtx_pair24[1] = &mtx4;

	/*mutex test with 2 threads */

	ret = lib_thread__create(&tid1, &priority__test, &mtx1, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(40);		// give just created thread some time to execute
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	temp_test_var = 0;
	ret = lib_thread__create(&tid2, &mutex__test, &mtx1, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	while (temp_test_var == 0) {
		lib_thread__msleep(10);
	}
	temp_test_var = 0;

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);


	/* test if  mutex_test thread is working */
	temp_test_var = 0;
	ret= lib_thread__create(&tid1, &mutex__test, &mtx1, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	while (temp_test_var == 0) {
		lib_thread__msleep(10);
	}
	temp_test_var = 0;

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_BUSY, ret);

	temp_test_var = 0;
//
    ret = lib_thread__join(&tid1, NULL) ; //(void*)&ret)
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(999);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	/* creation of a lock conditon if mutex_test is working */
	temp_test_var = 0;
	ret = lib_thread__create(&tid1, &mutex__test, &mtx1, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	while (temp_test_var == 0) {
		lib_thread__msleep(150);
	}
	temp_test_var = 0;

	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);


	/* Check if trylock is working */
	temp_test_var = 0;
	ret = lib_thread__create(&tid1, &mutex__test, &mtx1, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	while (temp_test_var == 0) {
		lib_thread__msleep(150);
	}

	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_BUSY, ret);

	temp_test_var = 0;

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	TEST_INFO("b. Basic Priority Inversion\n");

	// TC1: thread 2 locks mutex 1 and then waits, so that the lower-prio thread 1 can also lock it -> no priority inversion necessary for now
	// then, however, thread 3 also locks mutex 1 -> priority inversion necessary for thread 2
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);


	ret = lib_thread__create(&tid1, &mutex__test_1, &mtx1, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(200);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid3, &mutex__test_1, &mtx1, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC3: thread 1 locks both mutex 1 and 2, and threads 2 and 3 lock either mutex 1 or 2, respectively -> gradual priority inversion is employed and reversely reset
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_1, &mtx1, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_trylock(mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid2, &mutex__test_1, &mtx2, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC4: same as TC3 above, but with reversed mutex unlocking order in thread 1 -> same behavior as in TC2
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_1, &mtx1, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_trylock(mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid2, &mutex__test_1, &mtx2, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	TEST_INFO("c. Advanced Priority Inversion\n");

	// TC5: 4 threads (t) lock (l) several different mutexes: t1l1 -> t2l2l1 -> t3l3l2 -> t4l4l2 -> t5l4 -> ... -> gradual priority inversion is employed to all threads but t4 and immediately reset
	ret= lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret= lib_thread__create(&tid1, &mutex__test_2a, &mtx_pair12, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes

	ret= lib_thread__create(&tid2, &mutex__test_2a, &mtx_pair23, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes

	ret= lib_thread__create(&tid3, &mutex__test_2a, &mtx_pair24, 3, TASK_PRIO_PLUS3);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes

	ret= lib_thread__create(&tid4, &mutex__test_1, &mtx4, 4, TASK_PRIO_PLUS4);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes

	ret= lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret= lib_thread__join(&tid4, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret= lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret= lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret= lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC6: same as TC5 above, but with reversed mutex unlocking order in thread 1 -> same behavior as in TC5
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_2b, &mtx_pair12, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes

	ret = lib_thread__create(&tid2, &mutex__test_2b, &mtx_pair23, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes

	ret = lib_thread__create(&tid3, &mutex__test_2b, &mtx_pair24, 3, TASK_PRIO_PLUS3);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes

	ret = lib_thread__create(&tid4, &mutex__test_1, &mtx4, 4, TASK_PRIO_PLUS4);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid4, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC7: thread 2 locks mutex 1 and then waits, so that the lower-prio thread 1 can also lock mutex 2 and then mutex 1 -> no priority inversion necessary for now
	// then, however, thread 3 also locks mutex 2 -> priority inversion necessary for thread 1 AND 2, since thread 1 needs to wait for thread 2 before it can unlock mutex 2
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_2c, &mtx_pair12, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(200);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid3, &mutex__test_1, &mtx2, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC8: same as TC7 (with t4 instead of t3), but with preceding PI employed to thread 2 due to thread 3 locking mutex 1 before -> same behavior as in TC7 (t3 is "disregarded" for the time being)
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid3, &mutex__test_1, &mtx1, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_2d, &mtx_pair12, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(200);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid4, &mutex__test_1, &mtx2, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid4, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	struct condvar_wait_arg
	{
		cond_hdl_t *cond;
		mutex_hdl_t *mtx;
		int			prioidx;
	};	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC9: same as TC8, but with roles (and thereby priority) of t3 and t4 switched -> results in two actually independent "usual" priority inversion processes
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid4, &mutex__test_1, &mtx1, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_2e, &mtx_pair12, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(200);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid3, &mutex__test_1, &mtx2, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid4, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC10: same as TC7, but with another thread 4 locking mutex 2 after thread 3 -> t4 "replaces" t3, otherwise same behavior as in TC7 (t3 is "disregarded" for the time being)
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_2d, &mtx_pair12, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(200);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid3, &mutex__test_1, &mtx2, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid4, &mutex__test_1, &mtx2, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid4, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC11: combination of TC8 and TC10 -> same behavior as in TC8 and TC10 (t3 and t4 are "disregarded" for the time being)
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid3, &mutex__test_1, &mtx1, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_2f, &mtx_pair12, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(200);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid4, &mutex__test_1, &mtx2, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid5, &mutex__test_1, &mtx2, 3, TASK_PRIO_PLUS3);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid4, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid5, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC12: extension to TC7: t2 locks m1 and subsequently m3, which is already locked by (a waiting) t3;
	//	then t1 locks m2 and subsequently m1, causing it to block;
	//	then, t4 locks l2, causing a chained PI to take place: t1, t2 and t3 are elevated to the priority of t4
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid2, &mutex__test_2c, &mtx_pair12, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__test_2c, &mtx_pair23, -2, TASK_PRIO_MINUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(200);	// give low-prio thread some time to execute
	ret = lib_thread__create(&tid3, &mutex__test_1, &mtx3, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	// TC13: super test case formed by mixture of various test cases from above
	//	t4 locks m1 and waits -> t5 blocks on m1 and causes PI with t4 -> t3 locks m2 -> t6 blocks on m2 and causes PI with t3 -> t3 blocks on m1 and causes PI with t4, "resetting" t5 ->
	//	t1 locks m3 -> t2 blocks on m3 and causes PI with t1 -> t1 blocks on m2 -> t7 blocks on m3 and causes PI with t1, t3 and t4, "resetting" t2 and t6 ->
	//	t4 stops waiting and unlocks m1 -> t3 locks m1, then unlocks m1 and m2 -> t1 locks m2, then unlocks m2 and m3 -> t7 locks m3, then unlocks m3 -> t6 locks m2, then unlocks it ->
	//	t5 locks m1, then unlocks it -> t4 resumes execution -> t3 resumes execution -> t2 locks on m3, then unlocks it -> t1 resumes execution -> IDLE (finished)
	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid5, &mutex__test_1, &mtx1, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid3, &mutex__test_2g, &mtx_pair12, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(150);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid6, &mutex__test_1, &mtx2, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(150);	// give low-prio thread some time to execute

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
//	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);

	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &mutex__unlock_test, &mtx1, +1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_BUSY, ret);

	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(-EEXEC_DEADLK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_BUSY, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_INVAL, ret);

	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_trylock(mtx1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	/* Good Cases */

	ret = lib_thread__mutex_init(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx3);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx4);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	mtx_pair12[0] = &mtx1;	mtx_pair12[1] = &mtx2;
	mtx_pair23[0] = &mtx2;	mtx_pair23[1] = &mtx3;
	mtx_pair24[0] = &mtx2;	mtx_pair24[1] = &mtx4;

	/*mutex test with 2 threads */


	ret = lib_thread__create(&tid1, &mutex__test_2g, &mtx_pair23, -3, TASK_PRIO_MINUS3);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(150);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid2, &mutex__test_1, &mtx3, -2, TASK_PRIO_MINUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(150);	// give low-prio thread some time to execute

	ret = lib_thread__create(&tid7, &mutex__test_1, &mtx3, 3, TASK_PRIO_PLUS3);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(150);	// give low-prio thread some time to execute

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);
	//TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid7, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid6, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid5, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx4);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx3);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

static void test_lib_thread__signal(void)
{
	int ret;
	thread_hdl_t tid1, tid2, tid3;
	signal_hdl_t test_sgn, sgn;
	struct signal_helper sgn_test;


	ret = lib_thread__signal_init(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__signal_destroy(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__signal_send(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__signal_wait(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__signal_timedwait(NULL, 0);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__signal_init(&test_sgn);	// create dummy signal to get a valid signal object
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_destroy(&test_sgn);	// destroy signal to get an invalid signal object afterwards
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_destroy(&test_sgn);  // == -ESTD_INVAL);
	TEST_ASSERT_EQUAL_INT(-ESTD_INVAL, ret);

	ret = lib_thread__signal_send(test_sgn); 	 //	== -ESTD_INVAL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__signal_wait(test_sgn);     //== -ESTD_INVAL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__signal_timedwait(test_sgn, 0);   //== -ESTD_INVAL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	TEST_INFO("Basic Signal Handling\n");

	ret = lib_thread__signal_init(&sgn_test.sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid2, &signal_wait, (void*)&sgn_test.sgn, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid3, &signal_wait, (void*)&sgn_test.sgn, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &signal_send, (void*)&sgn_test, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_wait(sgn_test.sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_timedwait(sgn_test.sgn, 1000);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(2000);
	sgn_test.signal_send_running = 0;

	ret = lib_thread__signal_timedwait(sgn_test.sgn, 10);
	TEST_ASSERT_EQUAL_INT(-EEXEC_TO, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_destroy(&sgn_test.sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_init(&sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &signal_destroy, &sgn, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_wait(sgn);
	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_init(&sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &signal_destroy, &sgn, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_wait(sgn);
	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_init(&sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &signal_destroy, &sgn, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_wait(sgn);
	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_init(&sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &signal_destroy, &sgn, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_timedwait(sgn, 1000);
	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
//
	ret = lib_thread__signal_init(&sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &signal_destroy, &sgn, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_timedwait(sgn, 1000);
	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_init(&sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &signal_destroy, &sgn, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_timedwait(sgn, 1000);
	TEST_ASSERT_EQUAL_INT(-ESTD_PERM, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_init(&sgn);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &signal_destroy, &sgn, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__signal_timedwait(sgn, 10);
	TEST_ASSERT_EQUAL_INT(-EEXEC_TO, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

static void test_lib_thread__sem(void)
{
	thread_hdl_t tid1, tid2, tid3;
	sem_hdl_t sem;
	int ret;

	ret = lib_thread__sem_init(NULL, 0);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__sem_init(&sem, ((unsigned)SEM_VALUE_MAX)+1);
	TEST_ASSERT_EQUAL_INT(-ESTD_INVAL, ret);

	ret = lib_thread__sem_destroy(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__sem_wait(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__sem_trywait(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__sem_post(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__sem_init(&sem, 0);	// create dummy semaphore to get a valid semaphore object
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_destroy(&sem);	// destroy semaphore to get an invalid semaphore object afterwards
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_destroy(&sem);
	TEST_ASSERT_EQUAL_INT(-ESTD_INVAL, ret);

	ret = lib_thread__sem_wait(sem);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__sem_trywait(sem);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	TEST_INFO("Semaphore Functions\n");

	ret = lib_thread__sem_init(&sem, 1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_wait(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_wait(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_trywait(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_destroy(&sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_init(&sem, 0);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &semaphore_test, &sem, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid2, &semaphore_test, &sem, 1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid3, &semaphore_test, &sem, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(299);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_trywait(sem);
	TEST_ASSERT_EQUAL_INT(-ESTD_AGAIN, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_trywait(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_trywait(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_wait(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__sem_post(sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	lib_thread__msleep(1000);

	ret = lib_thread__sem_destroy(&sem);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid1, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid2, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__join(&tid3, NULL);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

static void test_lib_thread__condvar(void)
{
	int ret;
	thread_hdl_t tid1, tid2, tid3;
	mutex_hdl_t mtx1;
	cond_hdl_t cond1 = NULL;
	void* ret_arg;
	struct condvar_wait_arg	funcarg;

	ret = lib_thread__cond_init(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_destroy(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_signal(NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_wait(NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_wait(cond1, NULL);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_wait(NULL, mtx1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_timedwait(NULL, NULL, 100);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_timedwait(cond1, NULL, 100);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

    ret = lib_thread__cond_timedwait(NULL, mtx1, 100);

	/* TC_CONDVAR_INVAL1: prepare invalidate cond1 for sure, invalidate mtx1 for sure */
	ret = lib_thread__cond_init(&cond1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_destroy(&cond1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
    TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_destroy(&cond1);
	TEST_ASSERT_EQUAL_INT(-ESTD_INVAL, ret);

	ret = lib_thread__cond_signal(cond1);
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_wait(cond1, mtx1);						/* invalid condvar, invalid mtx */
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_timedwait(cond1, mtx1, 100);			/* invalid condvar, invalid mtx */
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	/* TC_CONDVAR_INVAL1: repeat with invalid condvar, valid mutex */
	ret = lib_thread__mutex_init(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_wait(cond1, mtx1);						/* invalid condvar, valid mtx */
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_timedwait(cond1, mtx1, 100);			/* invalid condvar, valid mtx */
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	/* TC_CONDVAR_INVAL1: repeat with valid condvar, invalid mutex */
	ret = lib_thread__cond_init(&cond1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_wait(cond1, mtx1);						/* valid condvar, invalid mtx */
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_timedwait(cond1, mtx1, 100);							/* valid condvar, invalid mtx */
	TEST_ASSERT_EQUAL_INT(-EPAR_NULL, ret);

	ret = lib_thread__cond_destroy(&cond1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	TEST_INFO("Condition Variable Functions\n");

	ret = lib_thread__cond_init(&cond1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_init(&mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	funcarg.cond = &cond1;
	funcarg.mtx  = &mtx1;

	ret = lib_thread__create(&tid1, &condvar_wait, &funcarg, +1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(100);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_signal(cond1);						/* task: cond_wait returns EOK */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(100);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_signal(cond1);						/* task: cond_timedwait returns EOK */

	ret = lib_thread__msleep(100);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

    lib_thread__join(&tid1, (void*)&ret_arg);
	TEST_ASSERT_EQUAL_INT(EOK, ret_arg);

	/* cond_wait: test concurrent access with context changes */
	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &condvar_thread_lockandsignal_sleep, &funcarg, +1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(100);	/* to ensure context changes to tid1 */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_wait(cond1, mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);


    ret = lib_thread__join(&tid1, (void**)&ret_arg);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	/* cond_timedwait: test concurrent access with context changes */
	ret = lib_thread__mutex_lock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__create(&tid1, &condvar_thread_lockandsignal_sleep, &funcarg, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_timedwait(cond1, mtx1, 1000);	/* must not(!) return with -EEXEC_TO since signal does not violate timeout, but mutex lock takes longer */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

    ret = lib_thread__join(&tid1, (void*)&ret_arg);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_unlock(mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	/* concurrent accesses with correct scheduling behavior */
	funcarg.prioidx = 0;
	ret = lib_thread__create(&tid1, &condvar_thread_waitandtimedwait, &funcarg, -1, TASK_PRIO_MINUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	funcarg.prioidx = 1;
	ret = lib_thread__create(&tid2, &condvar_thread_waitandtimedwait, &funcarg, +1, TASK_PRIO_PLUS1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	funcarg.prioidx = 2;
	ret = lib_thread__create(&tid3, &condvar_thread_waitandtimedwait, &funcarg, +2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(100);	/* make sure all tasks are at 'cond_wait' */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__cond_signal(cond1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(100);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	s_wup[0] == 0 && s_wup[1] == 0 && s_wup[2] == 1;

	ret = lib_thread__cond_signal(cond1);				/* check tid2 */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(100);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	s_wup[0] == 0 && s_wup[1] == 1 && s_wup[2] == 1;

	ret = lib_thread__cond_signal(cond1);				/* check tid1 */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__msleep(100);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	s_wup[0] == 0 && s_wup[1] == 1 && s_wup[2] == 1;

	ret = lib_thread__msleep(1000);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	s_wup[0] == 1 && s_wup[1] == 1 && s_wup[2] == 1;

	/* synchronize to new test */
	s_wup[0] = 0;
	s_wup[1] = 0;
	s_wup[2] = 0;

	ret = lib_thread__msleep(10000);
	TEST_ASSERT_EQUAL_INT(EOK, ret);


	ret = lib_thread__cond_signal(cond1);	/* check tid3 */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	s_wup[0] == 0 && s_wup[1] == 0 && s_wup[2] == 1;

	ret = lib_thread__cond_signal(cond1);			/* check tid2 */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	s_wup[0] == 0 && s_wup[1] == 1 && s_wup[2] == 1;

	ret = lib_thread__cond_signal(cond1);			/* check tid1 */
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	s_wup[0] == 0 && s_wup[1] == 1 && s_wup[2] == 1;

	ret = lib_thread__msleep(10);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	s_wup[0] == 1 && s_wup[1] == 1 && s_wup[2] == 1;

    ret = lib_thread__join(&tid1, (void*)&ret_arg);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

    lib_thread__join(&tid2, (void*)&ret);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

    lib_thread__join(&tid3, (void*)&ret);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	/* Test cleanup: ... mutex and condvar for upcoming tests */
	ret = lib_thread__cond_destroy(&cond1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx1);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	ret = lib_thread__mutex_destroy(&mtx2);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

static void test_lib_thread__wakeup_init(void)
{
	int ret;
	ret = lib_thread__wakeup_init();
	TEST_ASSERT_EQUAL_INT(EOK,ret);
}

//static void test_lib_thread__wakeup_init_shared(void)
//{
//	int ret;
//	s_test_lib = NULL;
//	s_test_lib = dlopen("/tmp/lib_wakeup_shared_g.so", RTLD_LAZY);
//	if (s_test_lib == NULL) {
//		TEST_INFO("Couldn't open mapping.so: %s\n",dlerror());
//	}
//	TEST_ASSERT_NOT_NULL(s_test_lib);
//
//	s_test_lib_interfaces.wakeup__init = dlsym(s_test_lib, "lib_wakeup_shared__init");
//	TEST_ASSERT_NOT_NULL(s_test_lib_interfaces.wakeup__init);
//
//	ret = (*s_test_lib_interfaces.wakeup__init)();
//	TEST_ASSERT_EQUAL_INT(EOK,ret);
//}

static void test_lib_thread__wakeup_create(void)
{
	int ret;
	wakeup_worker_running = 1;

	ret = lib_thread__wakeup_create(&wakeup_obj, 500);
	TEST_ASSERT_EQUAL_INT(EOK,ret);

	ret = lib_thread__create(&wakeup_worker_th,&test_lib_thread__wakeup,(void*)&wakeup_obj, 0, "test_lib_thread_wakeup_worker");
	TEST_ASSERT_EQUAL_INT(EOK,ret);

}

static void test_lib_thread__test_duration(void)
{
	lib_thread__msleep(10000);
}

static void test_lib_thread__wakeup_cleanup_fail (void)
{
	int ret;
	ret = lib_thread__wakeup_cleanup();
	TEST_ASSERT_EQUAL_INT(-EBUSY,ret);

}

static void test_lib_thread__wakeup_destroy(void)
{
	int ret;

	wakeup_worker_running = 0;

	ret = lib_thread__join(&wakeup_worker_th, NULL);
	TEST_ASSERT_EQUAL_INT(EOK,ret);

	ret = lib_thread__wakeup_destroy(&wakeup_obj);
	TEST_ASSERT_EQUAL_INT(EOK,ret);
}

//static void test_lib_thread__wakeup_cleanup_shared(void)
//{
//	int ret;
//
//	s_test_lib_interfaces.wakeup__cleanup = dlsym(s_test_lib, "lib_wakeup_shared__cleanup");
//	TEST_ASSERT_NOT_NULL(s_test_lib_interfaces.wakeup__cleanup);
//
//	ret = (*s_test_lib_interfaces.wakeup__cleanup)();
//	TEST_ASSERT_EQUAL_INT(EOK,ret);
//}


static void test_lib_thread__wakeup_cleanup (void)
{
	int ret;
	ret = lib_thread__wakeup_cleanup();
	TEST_ASSERT_EQUAL_INT(EOK,ret);

}


/* *******************************************************************
 * Global Functions
/* *******************************************************************/
TestRef lib_thread_test(void) {
    EMB_UNIT_TESTFIXTURES(fixtures) {
     	new_TestFixture("test_lib_thread__init", test_lib_thread__init),
     	new_TestFixture("test_lib_thread__wakeup_init", test_lib_thread__wakeup_init),
//    	new_TestFixture("test_lib_thread__create", test_lib_thread__create),
//		new_TestFixture("test_lib_thread__join",test_lib_thread__join),
//		new_TestFixture("test_lib_thread__getname", test_lib_thread__getname),
//		new_TestFixture("test_lib_thread__mutex", test_lib_thread__mutex),
//		new_TestFixture("test_lib_thread__signal",test_lib_thread__signal),
//		new_TestFixture("test_lib_thread__sem", test_lib_thread__sem),
//		new_TestFixture("test_lib_thread__condvar", test_lib_thread__condvar),
 //    	new_TestFixture("test_lib_thread__wakeup_init_shared", test_lib_thread__wakeup_init_shared),
     	new_TestFixture("test_lib_thread__wakeup_create", test_lib_thread__wakeup_create),
		new_TestFixture("test_lib_thread__test_duration", test_lib_thread__test_duration),
		new_TestFixture("test_lib_thread__wakeup_cleanup_fail", test_lib_thread__wakeup_cleanup_fail),
		new_TestFixture("test_lib_thread__wakeup_destroy", test_lib_thread__wakeup_destroy),
	//	new_TestFixture("test_lib_thread__wakeup_cleanup_shared", test_lib_thread__wakeup_cleanup_shared),
		new_TestFixture("test_lib_thread__wakeup_cleanup", test_lib_thread__wakeup_cleanup),
    };
    EMB_UNIT_TESTCALLER(lib_thread_test, "test_lib_thread_test", setUp, tearDown, fixtures);

    return (TestRef)&lib_thread_test;
}



static void* test_lib_thread__wakeup (void *_arg)
{
	int ret;
	uint32_t timestamp, time_diff;
	wakeup_hdl_t *wakeup_obj = (wakeup_hdl_t*)_arg;

	timestamp = lib_clock__get_time_ms();

	while (wakeup_worker_running)
	{
		ret = lib_thread__wakeup_wait(wakeup_obj);

		time_diff = lib_clock__get_time_since_ms(timestamp);

		timestamp = lib_clock__get_time_ms();
		if(time_diff > 500) {
			TEST_INFO("Wakeup_interval exceeded with %i\n",time_diff);
		}
		else {
			TEST_INFO("Wakeup %i ret %i \n",time_diff, ret);
		}

	}

	TEST_INFO("Wakeup interval error %i\n",ret);
	return NULL;
}

/* thread test */

static void* thread__test(void *_arg)
{
	int prio;
	int count;
	struct thread_test_attr *desc;
	if (_arg != NULL) {
		desc = (struct thread_test_attr *)_arg;
		desc->running=1;
		while(desc->running) {
			lib_thread__msleep(desc->interval);
		}
		return NULL;
	}
	else {
		for (count = 0; count < 2; count++) {
			lib_thread__msleep(1000);
		}
	}
	return NULL;
}

static void* join__test(void *_tid)
{
	int ret;
	thread_hdl_t *tid = (thread_hdl_t *)_tid;

	ret = lib_thread__join(tid, NULL);
	TEST_ASSERT_EQUAL_INT_TH(EOK, ret);

	return ret;
}

static void* thread__dummy(void *_arg)
{
	int ret;
	if (_arg != NULL){
		/* try to join a thread passed to us by the caller */
		ret = lib_thread__join(_arg, NULL);
//		TEST_ASSERT_EQUAL_INT(-EEXEC_DEADLK,ret);
	}
	return _arg;
}

/* mutex test */

static void* print__dummy(void *_dummy)
{
	if (_dummy != NULL){
		volatile unsigned dummy = *(volatile unsigned *)_dummy;
		msg(LOG_LEVEL_info, M_APP_LIB_THREAD_TEST_ID, "Dummy value is %u\n", dummy);
	}
	return NULL;
}

static void* priority__test(void *_mutex)
{
	int ret;
	unsigned i, j;
	static volatile unsigned dummy = 0;
	thread_hdl_t th = NULL;
	mutex_hdl_t *mtx = (mutex_hdl_t *)_mutex;

	ret = lib_thread__mutex_lock(*mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__create(&th, &print__dummy, (void *)&dummy, 2, TASK_PRIO_PLUS2);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	for (i = 0; i < ITER; i++)
		for (j = 0; j < ITER; j++)
			dummy = (i+1) * (j+1);
	ret = lib_thread__join(&th, NULL);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* mutex__test(void *_mutex)
{
	int ret;
	mutex_hdl_t *mtx = (mutex_hdl_t*)_mutex;

	ret = lib_thread__mutex_lock(*mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	temp_test_var = 1;

	while (temp_test_var) {
		ret = lib_thread__msleep(999);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);
	}
	ret = lib_thread__mutex_unlock(*mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* mutex__test_1(void *_mutex)
{
	int ret;
	mutex_hdl_t *mtx = (mutex_hdl_t*)_mutex;

	ret = lib_thread__mutex_lock(*mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* mutex__test_2a(void *_mutex_pair)
{
	int ret;

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex1);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex1);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex3);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex3);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else if (_mutex_pair == mtx_pair24){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex4 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex4);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex4);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else{
		TEST_ASSERT_NOT_NULL_TH(_mutex_pair);
	}

	return NULL;
}

static void* mutex__test_2b(void *_mutex_pair)
{
	int ret;

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex1);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex1);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];


		ret = lib_thread__mutex_lock(*mutex3);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex3);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else if (_mutex_pair == mtx_pair24){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex4 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex4);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex4);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else{
		TEST_ASSERT_NOT_NULL_TH(_mutex_pair);
	}
	return NULL;
}

static void* mutex__test_2c(void *_mutex_pair)
{
	int ret;

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex1);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex1);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex3);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex3);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);
	}else{
		TEST_ASSERT_NOT_NULL_TH(_mutex_pair);
	}

	return NULL;
}

static void* mutex__test_2d(void *_mutex_pair)
{
	int ret;
	mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
	mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];

	ret = lib_thread__mutex_lock(*mutex2);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_lock(*mutex1);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*mutex1);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*mutex2);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* mutex__test_2e(void *_mutex_pair)
{
	int ret;
	mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
	mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];

	ret = lib_thread__mutex_lock(*mutex2);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_lock(*mutex1);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*mutex1);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*mutex2);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* mutex__test_2f(void *_mutex_pair)
{
	int ret;
	mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
	mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];

	ret = lib_thread__mutex_lock(*mutex2);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_lock(*mutex1);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*mutex1);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*mutex2);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* mutex__test_2g(void *_mutex_pair)
{
	int ret;

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__msleep(200);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex1);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex1);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];

		ret = lib_thread__mutex_lock(*mutex3);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__msleep(200);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_lock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex2);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__mutex_unlock(*mutex3);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	}else{
		TEST_ASSERT_NOT_NULL_TH(_mutex_pair);
	}

	return NULL;
}

static void* mutex__unlock_test(void *_mutex)
{
	int ret;

	mutex_hdl_t *mtx = (mutex_hdl_t*)_mutex;

	/* try to unlock a mutex not owned by this thread */
	ret = lib_thread__mutex_unlock(*mtx);
	TEST_ASSERT_EQUAL_INT_TH(-ESTD_PERM,ret);

	return NULL;
}

static void* signal_send(void *_arg)
{
	int ret;
	struct signal_helper *sgn_test = (struct signal_helper *)_arg;

	sgn_test->signal_send_running = 1;

	while (sgn_test->signal_send_running) {
		ret = lib_thread__signal_send(sgn_test->sgn);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

		ret = lib_thread__msleep(99);
		TEST_ASSERT_EQUAL_INT_TH(EOK,ret);
	}

	return NULL;
}

static void* signal_wait(void *_arg)
{
	int ret;
	signal_hdl_t *sgn = (signal_hdl_t*)_arg;

	ret = lib_thread__signal_wait(*sgn);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* signal_timedwait(void *_arg)
{
	int ret;
	signal_hdl_t *sgn = (signal_hdl_t*)_arg;

	ret = lib_thread__signal_timedwait(*sgn, 10000);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* signal_destroy(void *_arg)
{
	int ret;
	signal_hdl_t *sgn = (signal_hdl_t*)_arg;

	ret = lib_thread__msleep(99);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__signal_destroy(sgn);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* condvar_wait(void *_arg)
{
	int ret;
	struct condvar_wait_arg *cond_wait = (struct condvar_wait_arg *)_arg;

	ret = lib_thread__mutex_lock(*cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__cond_wait(*cond_wait->cond, *cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__cond_timedwait(*cond_wait->cond, *cond_wait->mtx, 1000);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__cond_timedwait(*cond_wait->cond, *cond_wait->mtx, 1000);
	TEST_ASSERT_EQUAL_INT_TH(-EEXEC_TO,ret);

	ret = lib_thread__msleep(10000);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

//	ret = lib_thread__cond_signal(((struct condvar_wait_arg*)_arg)->cond) == EOK);						/* post signal to simulate it is in vain */
	ret = lib_thread__cond_timedwait(*cond_wait->cond, *cond_wait->mtx, 1000);	/* cond_timedwait: must timeout since signal shall be in vain */
	TEST_ASSERT_EQUAL_INT_TH(-EEXEC_TO,ret);

	ret = lib_thread__mutex_unlock(*cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* condvar_thread_lockandsignal_sleep(void *_arg)
{
	int ret;
	struct condvar_wait_arg *cond_wait = (struct condvar_wait_arg *)_arg;

	ret = lib_thread__mutex_lock(*cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__cond_signal(*cond_wait->cond);	/* send signal early, not violating timeout condition */
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__msleep(2000);											/* wait longer than timeout condition to check return value != -EEXEC_TO */
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_unlock(*cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* condvar_thread_waitandtimedwait(void *_arg)
{
	int ret, idx;
	struct condvar_wait_arg *cond_wait = (struct condvar_wait_arg *)_arg;
	idx = cond_wait->prioidx;
	/* cond_wait test case */
	ret = lib_thread__mutex_lock(*cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__cond_wait(*cond_wait->cond, *cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	s_wup[idx]++;
	ret = lib_thread__mutex_unlock(*cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	/* synchronize to new test */
	ret = lib_thread__msleep(10000);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__mutex_lock(*cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__cond_timedwait(*cond_wait->cond, *cond_wait->mtx, 2000);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	s_wup[idx]++;
	ret = lib_thread__mutex_unlock(*cond_wait->mtx);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

static void* semaphore_test(void *_arg)
{
	int ret;
	sem_hdl_t *sem = (sem_hdl_t *)_arg;

	ret = lib_thread__sem_wait(*sem);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__msleep(99);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__sem_trywait(*sem);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__msleep(99);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__sem_wait(*sem);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	ret = lib_thread__sem_post(*sem);
	TEST_ASSERT_EQUAL_INT_TH(EOK,ret);

	return NULL;
}

