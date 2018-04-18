/***************************************************************************//**
 *	\file	test.c
 *	\brief	test cases for POSIX lib_thread implementation
 *
 * ============================================================================
 *	Disclaimer:
 *	COPYRIGHT (c) MAN Diesel & Turbo SE, All Rights Reserved
 *	(Augsburg, Germany)
 *
 * ============================================================================
 *
 *	Project:	MAN | SaCoS
 *	Compiler:	GNU GCC / MS Visual C++
 *	Target:		PowerPC with QNX / ARM with Linux / x86 with Windows
 *
 ******************************************************************************/
/*
 *	02.04.2014	MK	creation
 *	09.04.2014	MK	extend test cases to cover priority inversion
 *	24.06.2014	MK	extend test cases to cover wakeup functionality
 *
 *	---
 *	AH: Alexander Holzmann, MAN Diesel & Turbo SE, www.mandieselturbo.com
 *	AL: Andreas Lehner, MAN Diesel & Turbo SE, www.mandieselturbo.com
 *	BCL: Bianca-Charlotte Liehr, MAN Diesel & Turbo SE, www.mandieselturbo.com
 *	BL: Bernd Lindenmayr, MAN Diesel & Turbo SE, www.mandieselturbo.com
 *	MK: Markus Kohler, MAN Diesel & Turbo SE, www.mandieselturbo.com
 *	MR: Martin Reichherzer, MAN Diesel & Turbo SE, www.mandieselturbo.com
 *	TW: Thomas Willetal, MAN Diesel & Turbo SE, www.mandieselturbo.com
 */

// ////////////////////////////////////////////////////////
// Includes
// ////////////////////////////////////////////////////////

/* c-runtime */
#include <signal.h>			// SIGRTMIN, SIGRTMAX
#include <stdio.h>			// printf(), fflush(), snprintf()
#include <string.h>			// strlen(), strcmp()

/* system */
#include <limits.h>			// SEM_VALUE_MAX (QNX only)
#include <pthread.h>		// POSIX thread and mutex functions and definitions
#include <sched.h>			// POSIX scheduling parameter functions and definitions
#include <semaphore.h>		// POSIX semaphore functions and definitions
#ifdef __gnu_linux__
#include <unistd.h>			// syscall(), read(), close()
#include <sys/syscall.h>	// SYS_gettid
#include <sys/types.h>		// pid_t, open()
#include <sys/stat.h>		// open()
#include <fcntl.h>			// open()
#elif defined _WIN32
#include "Windows.h"		// GetThreadPriority()
#endif /*__gnu_linux__ or _WIN32 */

/* own libs */
#include <lib_thread.h>
#include <man_errno.h>	// errno values
#include <lib_log.h>


// ////////////////////////////////////////////////////////
// Defines
// ////////////////////////////////////////////////////////
#define APP_THREAD_TEST__NAME	"TST"

// defines of exceeded(!) priority ranges and default process priorities
#ifdef __gnu_linux__

#define PMINE	0
#define PMAXE	100
#define PCUR	10
#elif defined _WIN32
#define PMINE	-16
#define PMAXE	16
#define PCUR	0
#elif defined __QNXNTO__
#define PMINE	0	// 0 is only allowed for idle thread
#define PMAXE	256
#define PCUR	10
#endif /*__gnu_linux__ or _WIN32 or __QNXNTO__ */

#define ITER 900UL	// number of dummy cycle iterations to create CPU load


#ifdef _WIN32
#define TID(__th) (void *)(__th)
#else /* other operating systems */
#define TID(__th) (void *)(__th)
#endif /* _WIN32 or other operating systems */

#define _TEST(_EXP)																										\
	do {																												\
		msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "L%.4u 0x%p: ("#_EXP")?%*s", __LINE__, TID(pthread_self()), 102-sizeof(#_EXP), (_EXP)?"OK":"ERROR");	\
		fflush(stderr);	lib_thread__msleep(10);																			\
		fflush(stdout);	lib_thread__msleep(10);																			\
	} while (0)


// ////////////////////////////////////////////////////////
// Types, Structs, Enums
// ////////////////////////////////////////////////////////

/* thread handle object structure */
struct internal_thread {
	pthread_t		thd;
};

/* signal handle object structure */
struct internal_signal {
	pthread_mutex_t	sig_mtx;				// internal mutex associated to the signal
	pthread_cond_t	sig_cnd;				// actual signal condition variable
	unsigned		num_of_waiting_threads;	// number of threads currently waiting on this signal
	unsigned		num_of_pending_signals;	// number of signals to be sent to unblock all waiting threads (usually equal to the above)
	unsigned		destroy;				// flag to indicate whether this signal is about to be destroyed
};


// ////////////////////////////////////////////////////////
// Static Function Prototypes
// ////////////////////////////////////////////////////////

static int get_cur_prio(void);

static void* thread_dummy(void *_arg);
static void* thread_test(void *_exp_prio);
static void* join_test(void *_tid);
static void* print_dummy(void *_dummy);
static void* PI_test(void *_mutex);
static void* mutex_test(void *_mutex);
static void* mutex_test_1(void *_mutex);
static void* mutex_test_2(void *_mutex_pair);
static void* mutex_test_2a(void *_mutex_pair);
static void* mutex_test_2b(void *_mutex_pair);
static void* mutex_test_2c(void *_mutex_pair);
static void* mutex_test_2d(void *_mutex_pair);
static void* mutex_test_2e(void *_mutex_pair);
static void* mutex_test_2f(void *_mutex_pair);
static void* mutex_test_2g(void *_mutex_pair);
static void* mutex_unlock_test(void *_mutex);
static void* signal_send(void *_arg);
static void* signal_wait(void *_arg);
static void* signal_timedwait(void *_arg);
static void* signal_destroy(void *_arg);
static void* condvar_wait(void *_arg);
static void* condvar_thread_lockandsignal_sleep(void *_arg);
static void* condvar_thread_waitandtimedwait(void *_arg);
static void* semaphore_test(void *_arg);


// ////////////////////////////////////////////////////////
// Static Variables
// ////////////////////////////////////////////////////////

static mutex_hdl_t *mtx_pair12[2], *mtx_pair23[2], *mtx_pair24[2];

static int s_wup[3] = {0, 0, 0};

struct condvar_wait_arg
{
	cond_hdl_t *cond;
	mutex_hdl_t *mtx;
	int			prioidx;
};

// ////////////////////////////////////////////////////////
// Global Functions
// ////////////////////////////////////////////////////////


#define TASK_PRIO_PLUS4 	"emstop"
#define TASK_PRIO_PLUS3 	"emstop_hw_protection"
#define TASK_PRIO_PLUS2 	"spi_transfer_ext_adc"
#define TASK_PRIO_PLUS1 	"adc_ext"
#define TASK_PRIO_MINUS1 	"system_control"
#define TASK_PRIO_MINUS2 	"alm_control"
#define TASK_PRIO_MINUS3 	"alm_operator"

int main(void){
	thread_hdl_t	tid1, tid2, tid3, tid4, tid5, tid6, tid7;
	mutex_hdl_t		mtx1, mtx2, mtx3, mtx4;
	cond_hdl_t		cond1;
	signal_hdl_t	sgn;
	sem_hdl_t		sem;
	wakeup_hdl_t	wuo1, wuo2, wuo3;
	int				base_prio, exp_prio;
	char			name[16];
	void			*ret;
	int 			lib_log__printcounter;
#ifdef __gnu_linux__ /* set round robin scheduling on Linux, as this is not the default here */
	struct sched_param	param;
	int					policy;

	errno = pthread_getschedparam(pthread_self(), &policy, &param);
	if (errno != 0){
		perror("pthread_getschedparam");
		return -1;
	}
	param.sched_priority = PCUR;
	errno = pthread_setschedparam(pthread_self(), SCHED_RR, &param);
	if (errno != 0){
		perror("pthread_setschedparam");
		msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "If this binary was launched directly, run it as superuser (sudo ...).");
		msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "If this binary was launched via eclipse, run eclipse as superuser (sudo eclipse).");
		return -1;
	}
#endif /* _WIN32 or __gnu_linux__ */
	base_prio = get_cur_prio();


	/* "Bad" cases ************************************************************/
	lib_log__printcounter = 0;
	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "*** lib_thread *** - Bad Cases");
	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, " %i. Thread Functions", ++lib_log__printcounter);


	_TEST(lib_thread__create(NULL,	&thread_test,	NULL,	1,			TASK_PRIO_PLUS1		) == -EPAR_NULL);
	_TEST(lib_thread__create(&tid1,	NULL,			NULL,	1,			TASK_PRIO_PLUS1		) == -EPAR_NULL);
	_TEST(lib_thread__create(&tid1,	&thread_test,	NULL,	PMAXE-PCUR,	TASK_PRIO_PLUS1		) == -EPAR_RANGE);
	_TEST(lib_thread__create(&tid1,	&thread_test,	NULL,	PMINE-PCUR,	TASK_PRIO_PLUS1		) == -EPAR_RANGE);
	_TEST(lib_thread__create(&tid1,	&thread_dummy,	&tid1,	-1,			TASK_PRIO_MINUS1		) == EOK);	// create dummy thread to get a valid thread object
	_TEST(lib_thread__msleep(40) == EOK);	// give just created thread some time to execute
	_TEST(lib_thread__getname(tid1, NULL, sizeof(name)	) == -EPAR_NULL);
	_TEST(lib_thread__getname(NULL, name, sizeof(name)	) == -ESTD_SRCH);
	_TEST(lib_thread__getname(tid1, name, 1				) == -ESTD_RANGE);
	_TEST(lib_thread__join(NULL, NULL) == -EPAR_NULL);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);	// wait for dummy thread termination to get an invalid thread object afterwards
	_TEST(lib_thread__join(&tid1, NULL) == -ESTD_SRCH);
	_TEST(lib_thread__cancel(NULL) == -ESTD_SRCH);
	_TEST(lib_thread__cancel(tid1) == -ESTD_SRCH);
	_TEST(lib_thread__getname(tid1, name, sizeof(name)) == -ESTD_SRCH);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Mutex Functions", ++lib_log__printcounter);

	_TEST(lib_thread__mutex_init(NULL) == -EPAR_NULL);
	_TEST(lib_thread__mutex_destroy(NULL) == -EPAR_NULL);
	_TEST(lib_thread__mutex_lock(NULL) == -EPAR_NULL);
	_TEST(lib_thread__mutex_trylock(NULL) == -EPAR_NULL);
	_TEST(lib_thread__mutex_unlock(NULL) == -EPAR_NULL);

	_TEST(lib_thread__mutex_init(&mtx1) == EOK);
	_TEST(lib_thread__mutex_unlock(&mtx1) == -ESTD_PERM);
	_TEST(lib_thread__mutex_lock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_unlock_test, &mtx1, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__mutex_trylock(&mtx1) == -ESTD_BUSY);
	_TEST(lib_thread__mutex_lock(&mtx1) == -EEXEC_DEADLK);
	_TEST(lib_thread__mutex_destroy(&mtx1) == -ESTD_BUSY);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx1) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx1) == -ESTD_INVAL);
	_TEST(lib_thread__mutex_lock(&mtx1) == -ESTD_INVAL);
	_TEST(lib_thread__mutex_trylock(&mtx1) == -ESTD_INVAL);
	_TEST(lib_thread__mutex_unlock(&mtx1) == -ESTD_INVAL);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Signal Functions", ++lib_log__printcounter);

	_TEST(lib_thread__signal_init(NULL) == -EPAR_NULL);
	_TEST(lib_thread__signal_destroy(NULL) == -EPAR_NULL);
	_TEST(lib_thread__signal_send(NULL) == -EPAR_NULL);
	_TEST(lib_thread__signal_wait(NULL) == -EPAR_NULL);
	_TEST(lib_thread__signal_timedwait(NULL, 0) == -EPAR_NULL);
	_TEST(lib_thread__signal_init(&sgn) == EOK);	// create dummy signal to get a valid signal object
	_TEST(lib_thread__signal_destroy(&sgn) == EOK);	// destroy signal to get an invalid signal object afterwards
	_TEST(lib_thread__signal_destroy(&sgn) == -ESTD_INVAL);
	_TEST(lib_thread__signal_send(&sgn) == -ESTD_INVAL);
	_TEST(lib_thread__signal_wait(&sgn) == -ESTD_INVAL);
	_TEST(lib_thread__signal_timedwait(&sgn, 0) == -ESTD_INVAL);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Semaphore Functions", ++lib_log__printcounter);

	/* TC_CONDVAR_NULL: call condvar functions with all possible combinations of NULL pointers */
	_TEST(lib_thread__cond_init(NULL) == -EPAR_NULL);
	_TEST(lib_thread__cond_destroy(NULL) == -EPAR_NULL);
	_TEST(lib_thread__cond_signal(NULL) == -EPAR_NULL);
	_TEST(lib_thread__cond_wait(NULL, NULL) == -EPAR_NULL);
	_TEST(lib_thread__cond_wait(&cond1, NULL) == -EPAR_NULL);
	_TEST(lib_thread__cond_wait(NULL, &mtx1) == -EPAR_NULL);
	_TEST(lib_thread__cond_timedwait(NULL, NULL, 100) == -EPAR_NULL);
	_TEST(lib_thread__cond_timedwait(&cond1, NULL, 100) == -EPAR_NULL);
	_TEST(lib_thread__cond_timedwait(NULL, &mtx1, 100) == -EPAR_NULL);

	/* TC_CONDVAR_INVAL1: prepare invalidate cond1 for sure, invalidate mtx1 for sure */
	_TEST(lib_thread__cond_init(&cond1) == EOK);
	_TEST(lib_thread__cond_destroy(&cond1) == EOK);
	_TEST(lib_thread__mutex_init(&mtx1) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx1) == EOK);

	_TEST(lib_thread__cond_destroy(&cond1) == -ESTD_INVAL);
	_TEST(lib_thread__cond_signal(&cond1) == -ESTD_INVAL);
	_TEST(lib_thread__cond_wait(&cond1, &mtx1) == -ESTD_INVAL);						/* invalid condvar, invalid mtx */
	_TEST(lib_thread__cond_timedwait(&cond1, &mtx1, 100) == -ESTD_INVAL);			/* invalid condvar, invalid mtx */

	/* TC_CONDVAR_INVAL1: repeat with invalid condvar, valid mutex */
	_TEST(lib_thread__mutex_init(&mtx1) == EOK);
	_TEST(lib_thread__cond_wait(&cond1, &mtx1) == -ESTD_INVAL);						/* invalid condvar, valid mtx */
	_TEST(lib_thread__cond_timedwait(&cond1, &mtx1, 100) == -ESTD_INVAL);			/* invalid condvar, valid mtx */
	_TEST(lib_thread__mutex_destroy(&mtx1) == EOK);

	/* TC_CONDVAR_INVAL1: repeat with valid condvar, invalid mutex */
	_TEST(lib_thread__cond_init(&cond1) == EOK);
	_TEST(lib_thread__cond_wait(&cond1, &mtx1) == -ESTD_INVAL);						/* valid condvar, invalid mtx */
	_TEST(lib_thread__cond_timedwait(&cond1, &mtx1, 100) == -ESTD_INVAL);			/* valid condvar, invalid mtx */
	_TEST(lib_thread__cond_destroy(&cond1) == EOK);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Semaphore Functions", ++lib_log__printcounter);

	_TEST(lib_thread__sem_init(NULL, 0) == -EPAR_NULL);
	_TEST(lib_thread__sem_init(&sem, ((unsigned)SEM_VALUE_MAX)+1) == -ESTD_INVAL);
	_TEST(lib_thread__sem_destroy(NULL) == -EPAR_NULL);
	_TEST(lib_thread__sem_wait(NULL) == -EPAR_NULL);
	_TEST(lib_thread__sem_trywait(NULL) == -EPAR_NULL);
	_TEST(lib_thread__sem_post(NULL) == -EPAR_NULL);
	_TEST(lib_thread__sem_init(&sem, 0) == EOK);	// create dummy semaphore to get a valid semaphore object
	_TEST(lib_thread__sem_destroy(&sem) == EOK);	// destroy semaphore to get an invalid semaphore object afterwards
	_TEST(lib_thread__sem_destroy(&sem) == -ESTD_INVAL);
	_TEST(lib_thread__sem_wait(&sem) == -ESTD_INVAL);
	_TEST(lib_thread__sem_trywait(&sem) == -ESTD_INVAL);
	_TEST(lib_thread__sem_post(&sem) == -ESTD_INVAL);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Wakeup Timer Functions", ++lib_log__printcounter);

	_TEST(lib_thread__wakeup_cleanup() == -EEXEC_NOINIT);
	_TEST(lib_thread__wakeup_create(&wuo1, 100) == -EEXEC_NOINIT);
	_TEST(lib_thread__wakeup_destroy(&wuo1) == -EEXEC_NOINIT);
	_TEST(lib_thread__wakeup_wait(&wuo1) == -EEXEC_NOINIT);
	_TEST(lib_thread__wakeup_init() == EOK);
	_TEST(lib_thread__wakeup_create(NULL, 100) == -EPAR_NULL);
	_TEST(lib_thread__wakeup_destroy(NULL) == -EPAR_NULL);
	_TEST(lib_thread__wakeup_wait(NULL) == -EPAR_NULL);
	_TEST(lib_thread__wakeup_destroy(&wuo1) == -ESTD_INVAL);
	_TEST(lib_thread__wakeup_wait(&wuo1) == -ESTD_INVAL);
	_TEST(lib_thread__wakeup_create(&wuo1, 100) == EOK);
	_TEST(lib_thread__wakeup_cleanup() == -ESTD_BUSY);
	_TEST(lib_thread__wakeup_destroy(&wuo1) == EOK);
	_TEST(lib_thread__wakeup_create(&wuo1, 0) == -ESTD_INVAL);
	_TEST(lib_thread__wakeup_destroy(&wuo1) == -ESTD_INVAL);
	_TEST(lib_thread__wakeup_wait(&wuo1) == -ESTD_INVAL);
#ifndef _WIN32	// different behavior on Linux, QNX and Windows
	{
		unsigned i;
		wakeup_hdl_t wuos[SIGRTMAX - SIGRTMIN + 1];

		for (i = 0; i < sizeof(wuos)/sizeof(*wuos); i++){
			_TEST(lib_thread__wakeup_create(wuos + i, 100) == EOK);
		}
		_TEST(lib_thread__wakeup_create(&wuo1, 100) == -ESTD_AGAIN);
		for (i = 0; i < sizeof(wuos)/sizeof(*wuos); i++){
			_TEST(lib_thread__wakeup_destroy(wuos + i) == EOK);
		}
	}
#endif /* _WIN32 or other operating systems */
	_TEST(lib_thread__wakeup_cleanup() == EOK);


	/* "Good" cases ***********************************************************/
	lib_log__printcounter = 0;
	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "*** lib_thread *** - Good Cases");
	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, " %i. Thread Functions", ++lib_log__printcounter);


	exp_prio = base_prio - 1;
	_TEST(lib_thread__create(&tid1, &thread_test, &exp_prio, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__getname(tid1, name, sizeof(name)) == EOK);
	_TEST(strcmp(name, TASK_PRIO_MINUS1) == 0);

	exp_prio = base_prio + 1;
	_TEST(lib_thread__create(&tid2, &thread_test, &exp_prio, 1, NULL) == EOK);
	_TEST(lib_thread__getname(tid2, name, sizeof(name)) == EOK);
	_TEST(name[0] == '\0');	/* must return 'empty string' on NULL pointer thread name */

	_TEST(pthread_detach(tid2->thd) == 0);
	_TEST(lib_thread__join(&tid2, NULL) == -ESTD_INVAL);
	_TEST(lib_thread__cancel(tid2) == EOK);
	_TEST(lib_thread__cancel(tid1) == EOK);
	_TEST(lib_thread__join(&tid1, &ret) == EOK);
	_TEST(ret == PTHREAD_CANCELED);

	/*
	 * This test case should cover the case that two concurrent joins are performed on the same thread.
	 *
	 * Therefore, a second thread is started after the first one with a higher priority than the main thread, interrupting the main thread
	 * immediately but then waiting for the first (low-priority) thread to terminate. Subsequently, the main thread starts running again,
	 * executing the "second" join command on the first (low-priority) thread, and is then expected to terminate with ESTD_INVAL due to
	 * the still pending join from the second (high-priority) thread.
	 */
	exp_prio = base_prio - 1;
	_TEST(lib_thread__create(&tid1, &thread_test, &exp_prio, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__create(&tid2, &join_test, &tid1, 4, TASK_PRIO_PLUS4) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == -ESTD_INVAL);
	_TEST(lib_thread__join(&tid2, &ret) == EOK);
	_TEST(ret == &exp_prio);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Mutex Functions", ++lib_log__printcounter);
	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "   a. Basic Functionality");

	lib_thread__fast_lock();
	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "Fast lock performed.");
	lib_thread__fast_unlock();
	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "Fast unlock performed.");
	_TEST(lib_thread__mutex_init(&mtx1) == EOK);
	_TEST(lib_thread__mutex_init(&mtx2) == EOK);
	_TEST(lib_thread__mutex_init(&mtx3) == EOK);
	_TEST(lib_thread__mutex_init(&mtx4) == EOK);
	mtx_pair12[0] = &mtx1;	mtx_pair12[1] = &mtx2;
	mtx_pair23[0] = &mtx2;	mtx_pair23[1] = &mtx3;
	mtx_pair24[0] = &mtx2;	mtx_pair24[1] = &mtx4;

	_TEST(lib_thread__create(&tid1, &PI_test, &mtx1, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__msleep(40) == EOK);	// give just created thread some time to execute
	_TEST(lib_thread__create(&tid2, &mutex_test, &mtx1, 2, TASK_PRIO_PLUS2) == EOK );
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);

	_TEST(lib_thread__create(&tid1, &mutex_test, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx1) == -ESTD_BUSY);
	_TEST(lib_thread__join(&tid1, &ret) == EOK);
	_TEST(ret == NULL);
	_TEST(lib_thread__msleep(999) == EOK);
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);

	_TEST(lib_thread__create(&tid1, &mutex_test, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__mutex_lock(&mtx1) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);

	_TEST(lib_thread__create(&tid1, &mutex_test, &mtx1, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__msleep(150) == EOK);	// give low-prio thread some time to execute
	_TEST(lib_thread__mutex_trylock(&mtx1) == -ESTD_BUSY);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);

// circular deadlock detection currently not supported on high-level OSes
/* 
	// lock m1
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	// create t2 which locks m2 and then m1 -> blocks on m1
	_TEST(lib_thread__create(&tid1, &mutex_test_2, &mtx_pair12, 1, TASK_PRIO_PLUS1) == EOK);
	// create t3 which locks m3 and then m2 -> blocks on m2
	_TEST(lib_thread__create(&tid2, &mutex_test_2, &mtx_pair23, 2, TASK_PRIO_PLUS2) == EOK);
	// lock m3 -> deadlock should occur
	_TEST(lib_thread__mutex_lock(&mtx3) == -EEXEC_DEADLK);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
*/


	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "  b. Basic Priority Inversion");

	// TC1: thread 2 locks mutex 1 and then waits, so that the lower-prio thread 1 can also lock it -> no priority inversion necessary for now
	// then, however, thread 3 also locks mutex 1 -> priority inversion necessary for thread 2
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_1, &mtx1, -1, TASK_PRIO_MINUS1) == EOK);
	lib_thread__msleep(200);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid3, &mutex_test_1, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	// TC2: 3 threads lock the same mutex sequentially in reverse order of their priority -> gradual priority inversion is employed and immediately reset
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_1, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__create(&tid2, &mutex_test_1, &mtx1, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	// TC3: thread 1 locks both mutex 1 and 2, and threads 2 and 3 lock either mutex 1 or 2, respectively -> gradual priority inversion is employed and reversely reset
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_1, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__mutex_trylock(&mtx2) == EOK);
	_TEST(lib_thread__create(&tid2, &mutex_test_1, &mtx2, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(&mtx2) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	// TC4: same as TC3 above, but with reversed mutex unlocking order in thread 1 -> same behavior as in TC2
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_1, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__mutex_trylock(&mtx2) == EOK);
	_TEST(lib_thread__create(&tid2, &mutex_test_1, &mtx2, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(&mtx2) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "  c. Advanced Priority Inversion");

	// TC5: 4 threads (t) lock (l) several different mutexes: t1l1 -> t2l2l1 -> t3l3l2 -> t4l4l2 -> t5l4 -> ... -> gradual priority inversion is employed to all threads but t4 and immediately reset
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_2a, &mtx_pair12, 1, TASK_PRIO_PLUS1) == EOK);
	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__create(&tid2, &mutex_test_2a, &mtx_pair23, 2, TASK_PRIO_PLUS2) == EOK);
	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__create(&tid3, &mutex_test_2a, &mtx_pair24, 3, TASK_PRIO_PLUS3) == EOK);
	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes
	_TEST(get_cur_prio() - base_prio == 3);
	_TEST(lib_thread__create(&tid4, &mutex_test_1, &mtx4, 4, TASK_PRIO_PLUS4) == EOK);
	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes
	_TEST(get_cur_prio() - base_prio == 4);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid4, NULL) == EOK);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	// TC6: same as TC5 above, but with reversed mutex unlocking order in thread 1 -> same behavior as in TC5
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_2b, &mtx_pair12, 1, TASK_PRIO_PLUS1) == EOK);
	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__create(&tid2, &mutex_test_2b, &mtx_pair23, 2, TASK_PRIO_PLUS2) == EOK);
	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__create(&tid3, &mutex_test_2b, &mtx_pair24, 3, TASK_PRIO_PLUS3) == EOK);
	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes
	_TEST(get_cur_prio() - base_prio == 3);
	_TEST(lib_thread__create(&tid4, &mutex_test_1, &mtx4, 4, TASK_PRIO_PLUS4) == EOK);
	lib_thread__msleep(100);	// prevent created thread from yielding back to the main thread (due to the console print commands inside) BEFORE locking the mutexes
	_TEST(get_cur_prio() - base_prio == 4);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid4, NULL) == EOK);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	// TC7: thread 2 locks mutex 1 and then waits, so that the lower-prio thread 1 can also lock mutex 2 and then mutex 1 -> no priority inversion necessary for now
	// then, however, thread 3 also locks mutex 2 -> priority inversion necessary for thread 1 AND 2, since thread 1 needs to wait for thread 2 before it can unlock mutex 2
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_2c, &mtx_pair12, -1, TASK_PRIO_MINUS1) == EOK);
	lib_thread__msleep(200);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid3, &mutex_test_1, &mtx2, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	// TC8: same as TC7 (with t4 instead of t3), but with preceding PI employed to thread 2 due to thread 3 locking mutex 1 before -> same behavior as in TC7 (t3 is "disregarded" for the time being)
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid3, &mutex_test_1, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_2d, &mtx_pair12, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	lib_thread__msleep(200);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid4, &mutex_test_1, &mtx2, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid4, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);

	// TC9: same as TC8, but with roles (and thereby priority) of t3 and t4 switched -> results in two actually independent "usual" priority inversion processes
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid4, &mutex_test_1, &mtx1, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_2e, &mtx_pair12, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	lib_thread__msleep(200);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid3, &mutex_test_1, &mtx2, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__join(&tid4, NULL) == EOK);

	// TC10: same as TC7, but with another thread 4 locking mutex 2 after thread 3 -> t4 "replaces" t3, otherwise same behavior as in TC7 (t3 is "disregarded" for the time being)
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_2d, &mtx_pair12, -1, TASK_PRIO_MINUS1) == EOK);
	lib_thread__msleep(200);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid3, &mutex_test_1, &mtx2, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__create(&tid4, &mutex_test_1, &mtx2, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid4, NULL) == EOK);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	// TC11: combination of TC8 and TC10 -> same behavior as in TC8 and TC10 (t3 and t4 are "disregarded" for the time being)
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid3, &mutex_test_1, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_2f, &mtx_pair12, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	lib_thread__msleep(200);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid4, &mutex_test_1, &mtx2, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__create(&tid5, &mutex_test_1, &mtx2, 3, TASK_PRIO_PLUS3) == EOK);
	_TEST(get_cur_prio() - base_prio == 3);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid4, NULL) == EOK);
	_TEST(lib_thread__join(&tid5, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);

	// TC12: extension to TC7: t2 locks m1 and subsequently m3, which is already locked by (a waiting) t3;
	//	then t1 locks m2 and subsequently m1, causing it to block;
	//	then, t4 locks l2, causing a chained PI to take place: t1, t2 and t3 are elevated to the priority of t4
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid2, &mutex_test_2c, &mtx_pair12, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__create(&tid1, &mutex_test_2c, &mtx_pair23, -2, TASK_PRIO_MINUS2) == EOK);
	lib_thread__msleep(200);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid3, &mutex_test_1, &mtx3, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);

	// TC13: super test case formed by mixture of various test cases from above
	//	t4 locks m1 and waits -> t5 blocks on m1 and causes PI with t4 -> t3 locks m2 -> t6 blocks on m2 and causes PI with t3 -> t3 blocks on m1 and causes PI with t4, "resetting" t5 ->
	//	t1 locks m3 -> t2 blocks on m3 and causes PI with t1 -> t1 blocks on m2 -> t7 blocks on m3 and causes PI with t1, t3 and t4, "resetting" t2 and t6 ->
	//	t4 stops waiting and unlocks m1 -> t3 locks m1, then unlocks m1 and m2 -> t1 locks m2, then unlocks m2 and m3 -> t7 locks m3, then unlocks m3 -> t6 locks m2, then unlocks it ->
	//	t5 locks m1, then unlocks it -> t4 resumes execution -> t3 resumes execution -> t2 locks on m3, then unlocks it -> t1 resumes execution -> IDLE (finished)
	_TEST(lib_thread__mutex_trylock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid5, &mutex_test_1, &mtx1, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(get_cur_prio() - base_prio == 1);
	_TEST(lib_thread__create(&tid3, &mutex_test_2g, &mtx_pair12, -1, TASK_PRIO_MINUS1) == EOK);
	lib_thread__msleep(150);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid6, &mutex_test_1, &mtx2, 2, TASK_PRIO_PLUS2) == EOK);
	lib_thread__msleep(150);	// give low-prio thread some time to execute
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__create(&tid1, &mutex_test_2g, &mtx_pair23, -3, TASK_PRIO_MINUS3) == EOK);
	lib_thread__msleep(150);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid2, &mutex_test_1, &mtx3, -2, TASK_PRIO_MINUS2) == EOK);
	lib_thread__msleep(150);	// give low-prio thread some time to execute
	_TEST(lib_thread__create(&tid7, &mutex_test_1, &mtx3, 3, TASK_PRIO_PLUS3) == EOK);
	lib_thread__msleep(150);	// give low-prio thread some time to execute
	_TEST(get_cur_prio() - base_prio == 3);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__join(&tid7, NULL) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__join(&tid6, NULL) == EOK);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__join(&tid5, NULL) == EOK);

	_TEST(lib_thread__mutex_destroy(&mtx4) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx3) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx2) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx1) == EOK);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Signal Functions", ++lib_log__printcounter);

	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__signal_send(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_send, &sgn, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__create(&tid2, &signal_wait, &sgn, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(lib_thread__create(&tid3, &signal_wait, &sgn, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__signal_wait(&sgn) == EOK);
	_TEST(lib_thread__signal_timedwait(&sgn, 1000) == EOK);
	_TEST(lib_thread__signal_timedwait(&sgn, 10) == -EEXEC_TO);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);
	_TEST(lib_thread__signal_destroy(&sgn) == EOK);

	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_destroy, &sgn, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__signal_wait(&sgn) == -ESTD_PERM);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_destroy, &sgn, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__signal_wait(&sgn) == -ESTD_PERM);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_destroy, &sgn, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(lib_thread__signal_wait(&sgn) == -ESTD_PERM);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_destroy, &sgn, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__signal_timedwait(&sgn, 1000) == -ESTD_PERM);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_destroy, &sgn, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__signal_timedwait(&sgn, 1000) == -ESTD_PERM);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_destroy, &sgn, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(lib_thread__signal_timedwait(&sgn, 1000) == -ESTD_PERM);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_destroy, &sgn, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__signal_timedwait(&sgn, 10) == -EEXEC_TO);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);

/*	_TEST(lib_thread__signal_init(&sgn) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_wait, &sgn, 1, TASK_PRIO_PLUS1) == EOK);	requires working cancellation of blocked threads
	_TEST(lib_thread__create(&tid2, &thread_dummy, &tid1, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(lib_thread__cancel(tid2) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(lib_thread__cancel(tid1) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__create(&tid1, &signal_timedwait, &sgn, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__cancel(tid1) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__signal_destroy(&sgn) == EOK);
*/

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Condition Variable Functions", ++lib_log__printcounter);
	/* Test prepare: Create valid mutex and condvar for further use */

	_TEST(lib_thread__cond_init(&cond1) == EOK);
	_TEST(lib_thread__mutex_init(&mtx1) == EOK);
	_TEST(lib_thread__mutex_init(&mtx2) == EOK);

#ifdef OSEK
	_TEST(lib_thread__cond_init(&cond1) == -ESTD_BUSY); 					/* cond_init: already initialized */
#endif

	struct condvar_wait_arg	funcarg;
	funcarg.cond = &cond1;
	funcarg.mtx  = &mtx1;
	_TEST(lib_thread__create(&tid1, &condvar_wait, &funcarg, +1, TASK_PRIO_PLUS1) == EOK);
//	_TEST(lib_thread__cond_destroy(&cond1) == -ESTD_BUSY);					/* cond_destroy: another task waiting */
	_TEST(lib_thread__msleep(100) == EOK);
//	_TEST(lib_thread__mutex_lock(&mtx2) == EOK);
//	_TEST(lib_thread__cond_wait(&cond1, &mtx2) == -ESTD_INVAL);				/* cond_wait: different mutexes supplied */
//	_TEST(lib_thread__cond_timedwait(&cond1, &mtx2, 100) == -ESTD_INVAL);	/* cond_timedwait: different mutexes supplied */
//	_TEST(lib_thread__mutex_unlock(&mtx2) == EOK);
	_TEST(lib_thread__cond_signal(&cond1) == EOK);						/* task: cond_wait returns EOK */
	_TEST(lib_thread__msleep(100) == EOK);
	_TEST(lib_thread__cond_signal(&cond1) == EOK);						/* task: cond_timedwait returns EOK */
	_TEST(lib_thread__msleep(1000) == EOK);								/* task: cond_timedwait returns -EEXEC_TO */
																		/* task now simulates 1s of work */

//	_TEST(lib_thread__cond_wait(&cond1, &mtx1) == -ESTD_PERM);				/* cond_wait: not owner of mutex (other thread holding mtx1) */
//	_TEST(lib_thread__cond_timedwait(&cond1, &mtx1, 100) == -ESTD_PERM);	/* cond_timedwait: not owner of mutex (other thread holding mtx1) */
//	lib_thread__mutex_unlock(&mtx1);
	_TEST(lib_thread__join(&tid1, &ret) == EOK);
//	_TEST(lib_thread__cond_wait(&cond1, &mtx1) == -ESTD_PERM);				/* cond_wait: not owner of mutex (noone holding mtx1)*/
//	_TEST(lib_thread__cond_timedwait(&cond1, &mtx1, 100) == -ESTD_PERM);	/* cond_timedwait: not owner of mutex (noone holding mtx1)*/

	/* cond_wait: test concurrent access with context changes */
	_TEST(lib_thread__mutex_lock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &condvar_thread_lockandsignal_sleep, &funcarg, +1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__msleep(100) == EOK);	/* to ensure context changes to tid1 */
	_TEST(lib_thread__cond_wait(&cond1, &mtx1) == EOK);
	_TEST(lib_thread__join(&tid1, &ret) == EOK);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);

	/* cond_timedwait: test concurrent access with context changes */
	_TEST(lib_thread__mutex_lock(&mtx1) == EOK);
	_TEST(lib_thread__create(&tid1, &condvar_thread_lockandsignal_sleep, &funcarg, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__cond_timedwait(&cond1, &mtx1, 1000) == EOK);	/* must not(!) return with -EEXEC_TO since signal does not violate timeout, but mutex lock takes longer */
	_TEST(lib_thread__join(&tid1, &ret) == EOK);
	_TEST(lib_thread__mutex_unlock(&mtx1) == EOK);

	/* concurrent accesses with correct scheduling behavior */
	funcarg.prioidx = 0;
	_TEST(lib_thread__create(&tid1, &condvar_thread_waitandtimedwait, &funcarg, -1, TASK_PRIO_MINUS1) == EOK);

	funcarg.prioidx = 1;
	_TEST(lib_thread__create(&tid2, &condvar_thread_waitandtimedwait, &funcarg, +1, TASK_PRIO_PLUS1) == EOK);

	funcarg.prioidx = 2;
	_TEST(lib_thread__create(&tid3, &condvar_thread_waitandtimedwait, &funcarg, +2, TASK_PRIO_PLUS2) == EOK);

	_TEST(lib_thread__msleep(100) == EOK);	/* make sure all tasks are at 'cond_wait' */

	_TEST(lib_thread__cond_signal(&cond1) == EOK);				/* check tid3 */
	_TEST(lib_thread__msleep(1000) == EOK);
	_TEST(s_wup[0] == 0 && s_wup[1] == 0 && s_wup[2] == 1);

	_TEST(lib_thread__cond_signal(&cond1) == EOK);				/* check tid2 */
	_TEST(lib_thread__msleep(1000) == EOK);
	_TEST(s_wup[0] == 0 && s_wup[1] == 1 && s_wup[2] == 1);

	_TEST(lib_thread__cond_signal(&cond1) == EOK);				/* check tid1 */
	_TEST(lib_thread__msleep(1000) == EOK);
	_TEST(s_wup[0] == 0 && s_wup[1] == 1 && s_wup[2] == 1);
	_TEST(lib_thread__msleep(1000) == EOK);
	_TEST(s_wup[0] == 1 && s_wup[1] == 1 && s_wup[2] == 1);

	/* synchronize to new test */
	s_wup[0] = 0;
	s_wup[1] = 0;
	s_wup[2] = 0;
	_TEST(lib_thread__msleep(10000) == EOK);

	_TEST(lib_thread__cond_signal(&cond1) == EOK);				/* check tid3 */
	_TEST(s_wup[0] == 0 && s_wup[1] == 0 && s_wup[2] == 1);

	_TEST(lib_thread__cond_signal(&cond1) == EOK);				/* check tid2 */
	_TEST(s_wup[0] == 0 && s_wup[1] == 1 && s_wup[2] == 1);

	_TEST(lib_thread__cond_signal(&cond1) == EOK);				/* check tid1 */
	_TEST(s_wup[0] == 0 && s_wup[1] == 1 && s_wup[2] == 1);
	_TEST(lib_thread__msleep(10) == EOK);
	_TEST(s_wup[0] == 1 && s_wup[1] == 1 && s_wup[2] == 1);

	_TEST(lib_thread__join(&tid1, &ret) == EOK);
	_TEST(lib_thread__join(&tid2, &ret) == EOK);
	_TEST(lib_thread__join(&tid3, &ret) == EOK);

	/* Test cleanup: ... mutex and condvar for upcoming tests */
	_TEST(lib_thread__cond_destroy(&cond1) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx1) == EOK);
	_TEST(lib_thread__mutex_destroy(&mtx2) == EOK);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Semaphore Functions", ++lib_log__printcounter);

	_TEST(lib_thread__sem_init(&sem, 1) == EOK);
	_TEST(lib_thread__sem_wait(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_wait(&sem) == EOK);
	_TEST(lib_thread__sem_trywait(&sem) == EOK);
	_TEST(lib_thread__sem_destroy(&sem) == EOK);
	_TEST(lib_thread__sem_init(&sem, 0) == EOK);
	_TEST(lib_thread__create(&tid1, &semaphore_test, &sem, -1, TASK_PRIO_MINUS1) == EOK);
	_TEST(lib_thread__create(&tid2, &semaphore_test, &sem, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__create(&tid3, &semaphore_test, &sem, 2, TASK_PRIO_PLUS2) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__msleep(299) == EOK);
	_TEST(lib_thread__sem_trywait(&sem) == -ESTD_AGAIN);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_post(&sem) == EOK);
	_TEST(lib_thread__sem_trywait(&sem) == EOK);
	_TEST(lib_thread__sem_trywait(&sem) == EOK);
	_TEST(lib_thread__sem_wait(&sem) == EOK);
	_TEST(lib_thread__sem_destroy(&sem) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__join(&tid2, NULL) == EOK);
	_TEST(lib_thread__join(&tid3, NULL) == EOK);

	_TEST(lib_thread__sem_init(&sem, 0) == EOK);
	_TEST(lib_thread__create(&tid1, &semaphore_test, &sem, 1, TASK_PRIO_PLUS1) == EOK);
	_TEST(lib_thread__cancel(tid1) == EOK);
	_TEST(lib_thread__join(&tid1, NULL) == EOK);
	_TEST(lib_thread__sem_destroy(&sem) == EOK);

	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "%i. Wakeup Timer Functions", ++lib_log__printcounter);

	_TEST(lib_thread__wakeup_init() == EOK);
	_TEST(lib_thread__wakeup_init() == EOK);
	_TEST(lib_thread__wakeup_create(&wuo1, 1) == EOK);
	_TEST(lib_thread__wakeup_create(&wuo2, 100) == EOK);
	_TEST(lib_thread__wakeup_create(&wuo3, 10000) == EOK);
	_TEST(lib_thread__wakeup_cleanup() == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo1) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo1) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo1) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo2) == EOK);
	_TEST(lib_thread__wakeup_destroy(&wuo1) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo2) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo2) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo2) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo3) == EOK);
	_TEST(lib_thread__wakeup_destroy(&wuo2) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo3) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo3) == EOK);
	_TEST(lib_thread__wakeup_wait(&wuo3) == EOK);
	_TEST(lib_thread__wakeup_destroy(&wuo3) == EOK);
	_TEST(lib_thread__wakeup_cleanup() == EOK);


	/* Finished ***************************************************************/
	msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "*** lib_thread *** - Finished");


	return 0;
}


// ////////////////////////////////////////////////////////
// Static Functions
// ////////////////////////////////////////////////////////

static int get_cur_prio(void){
#ifdef __gnu_linux__	// different behavior on all tested operating systems
	unsigned whitespace_count, is_negative;
	int i, fd, bytes_read, prio;
	char filename[80];
	char data[256];


	snprintf(filename, sizeof(filename), "/proc/%ld/stat", syscall(SYS_gettid));
	fd = open(filename, O_RDONLY, 0);
	if (fd == -1){
		return -0xFFFF;
	}

	bytes_read = read(fd, data, sizeof(data));
	if (bytes_read == sizeof(data)){
		close(fd);
		return -0xFFFF;
	}
	if (close(fd) != 0){
		return -0xFFFF;
	}
	data[bytes_read] = '\0';

	for (i = 0; i < bytes_read; i++){
		if ((data[i-1] == ')') && (data[i] == ' ')){
			break;
		}
	}
	for (prio = 0, whitespace_count = is_negative = 0; i < bytes_read; i++){
		if (whitespace_count == 16){
			if (data[i] == ' '){
				if (data[i-1] == ' '){
					return -0xFFFF;
				}
				break;
			}

			if (data[i] == '-'){
				is_negative = 1;
			}else if (is_negative){
				prio = prio*10 - (data[i] - '0');
			}else{
				prio = prio*10 + (data[i] - '0');
			}
		}else if (data[i] == ' '){
			whitespace_count++;
		}
	}


	return -prio;
#elif defined _WIN32
	int prio;


	prio = GetThreadPriority(GetCurrentThread());
	if (prio == THREAD_PRIORITY_ERROR_RETURN){
		return -0xFFFF;
	}


	return prio;
#elif defined __QNXNTO__
	struct sched_param param;
	int policy;


	if (pthread_getschedparam(pthread_self(), &policy, &param) != 0){
		return -0xFFFF;
	}


	return param.sched_curpriority;
#endif /*__gnu_linux__ or _WIN32 or __QNXNTO__ */
}



static void* thread_dummy(void *_arg){

	if (_arg != NULL){
		/* try to join a thread passed to us by the caller */
		_TEST(lib_thread__join(_arg, NULL) == -EEXEC_DEADLK);
	}


	return _arg;
}

static void* thread_test(void *_exp_prio){

	if (_exp_prio != NULL){
		_TEST(get_cur_prio() == *(int *)_exp_prio);
	}

	_TEST(lib_thread__msleep(999) == EOK);
	pthread_testcancel();


	return _exp_prio;
}

static void* join_test(void *_tid){
	void *ret;


	_TEST(lib_thread__join(_tid, &ret) == EOK);


	return ret;
}

static void* print_dummy(void *_dummy){

	if (_dummy != NULL){
		volatile unsigned dummy = *(volatile unsigned *)_dummy;
		msg(VERBOSITY_HIG, APP_THREAD_TEST__NAME, "Dummy value is %u", dummy);
	}


	return NULL;
}

static void* PI_test(void *_mutex){
	unsigned i, j;
	static volatile unsigned dummy = 0;
	thread_hdl_t th;


	_TEST(lib_thread__mutex_lock(_mutex) == EOK);
	_TEST(lib_thread__create(&th, &print_dummy, (void *)&dummy, 2, TASK_PRIO_PLUS1) == EOK);
	for (i = 0; i < ITER; i++)
		for (j = 0; j < ITER; j++)
			dummy = (i+1) * (j+1);
	_TEST(lib_thread__join(&th, NULL) == EOK);
	_TEST(lib_thread__mutex_unlock(_mutex) == EOK);


	return NULL;
}

static void* mutex_test(void *_mutex){

	_TEST(lib_thread__mutex_lock(_mutex) == EOK);
	_TEST(lib_thread__msleep(999) == EOK);
	_TEST(lib_thread__mutex_unlock(_mutex) == EOK);


	return NULL;
}

static void* mutex_test_1(void *_mutex){
	int base_prio = get_cur_prio();


	_TEST(lib_thread__mutex_lock(_mutex) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__mutex_unlock(_mutex) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);


	return NULL;
}

static void* mutex_test_2(void *_mutex_pair){

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];

		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(lib_thread__mutex_lock(mutex1) == EOK);
		_TEST(lib_thread__mutex_unlock(mutex1) == EOK);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];

		_TEST(lib_thread__mutex_lock(mutex3) == EOK);
		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(lib_thread__mutex_unlock(mutex3) == EOK);
	}else if (_mutex_pair == mtx_pair24){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex4 = ((mutex_hdl_t**)_mutex_pair)[1];

		_TEST(lib_thread__mutex_lock(mutex4) == EOK);
		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(lib_thread__mutex_unlock(mutex4) == EOK);
	}else{
		_TEST(_mutex_pair == NULL);
	}


	return NULL;
}

static void* mutex_test_2a(void *_mutex_pair){

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__mutex_lock(mutex1) == EOK);
		_TEST(get_cur_prio() - base_prio == 3);
		_TEST(lib_thread__mutex_unlock(mutex1) == EOK);
		_TEST(get_cur_prio() - base_prio == 3);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex3) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);	// no priority inversion here any more (since the actual PI causing thread has already finished his work)
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);	// no priority inversion here any more (since the actual PI causing thread has already finished his work)
		_TEST(lib_thread__mutex_unlock(mutex3) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else if (_mutex_pair == mtx_pair24){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex4 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex4) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 1);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 1);
		_TEST(lib_thread__mutex_unlock(mutex4) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else{
		_TEST(_mutex_pair == NULL);
	}


	return NULL;
}

static void* mutex_test_2b(void *_mutex_pair){

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__mutex_lock(mutex1) == EOK);
		_TEST(get_cur_prio() - base_prio == 3);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);	// no priority inversion here any more (since the actual PI causing thread has already finished his work)
		_TEST(lib_thread__mutex_unlock(mutex1) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex3) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);	// no priority inversion here any more (since the actual PI causing thread has already finished his work)
		_TEST(lib_thread__mutex_unlock(mutex3) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);	// no priority inversion here any more (since the actual PI causing thread has already finished his work)
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else if (_mutex_pair == mtx_pair24){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex4 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex4) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 1);
		_TEST(lib_thread__mutex_unlock(mutex4) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);	// no priority inversion here any more (since the actual PI causing thread has already finished his work)
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else{
		_TEST(_mutex_pair == NULL);
	}


	return NULL;
}

static void* mutex_test_2c(void *_mutex_pair){

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__mutex_lock(mutex1) == EOK);
		_TEST(get_cur_prio() - base_prio == 2);
		_TEST(lib_thread__mutex_unlock(mutex1) == EOK);
		_TEST(get_cur_prio() - base_prio == 2);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex3) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 3);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 3);
		_TEST(lib_thread__mutex_unlock(mutex3) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else{
		_TEST(_mutex_pair == NULL);
	}


	return NULL;
}

static void* mutex_test_2d(void *_mutex_pair){
	mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
	mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];
	int base_prio = get_cur_prio();


	_TEST(lib_thread__mutex_lock(mutex2) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__mutex_lock(mutex1) == EOK);
	_TEST(get_cur_prio() - base_prio == 3);
	_TEST(lib_thread__mutex_unlock(mutex1) == EOK);
	_TEST(get_cur_prio() - base_prio == 3);
	_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);


	return NULL;
}

static void* mutex_test_2e(void *_mutex_pair){
	mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
	mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];
	int base_prio = get_cur_prio();


	_TEST(lib_thread__mutex_lock(mutex2) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__mutex_lock(mutex1) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(mutex1) == EOK);
	_TEST(get_cur_prio() - base_prio == 2);
	_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);


	return NULL;
}

static void* mutex_test_2f(void *_mutex_pair){
	mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
	mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];
	int base_prio = get_cur_prio();


	_TEST(lib_thread__mutex_lock(mutex2) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);
	_TEST(lib_thread__mutex_lock(mutex1) == EOK);
	_TEST(get_cur_prio() - base_prio == 4);
	_TEST(lib_thread__mutex_unlock(mutex1) == EOK);
	_TEST(get_cur_prio() - base_prio == 4);
	_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
	_TEST(get_cur_prio() - base_prio == 0);


	return NULL;
}

static void* mutex_test_2g(void *_mutex_pair){

	if (_mutex_pair == mtx_pair12){
		mutex_hdl_t* mutex1 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__msleep(200) == EOK);
		_TEST(get_cur_prio() - base_prio == 3);
		_TEST(lib_thread__mutex_lock(mutex1) == EOK);
		_TEST(get_cur_prio() - base_prio == 4);
		_TEST(lib_thread__mutex_unlock(mutex1) == EOK);
		_TEST(get_cur_prio() - base_prio == 4);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else if (_mutex_pair == mtx_pair23){
		mutex_hdl_t* mutex2 = ((mutex_hdl_t**)_mutex_pair)[0];
		mutex_hdl_t* mutex3 = ((mutex_hdl_t**)_mutex_pair)[1];
		int base_prio = get_cur_prio();

		_TEST(lib_thread__mutex_lock(mutex3) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
		_TEST(lib_thread__msleep(200) == EOK);
		_TEST(get_cur_prio() - base_prio == 1);
		_TEST(lib_thread__mutex_lock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 6);
		_TEST(lib_thread__mutex_unlock(mutex2) == EOK);
		_TEST(get_cur_prio() - base_prio == 6);
		_TEST(lib_thread__mutex_unlock(mutex3) == EOK);
		_TEST(get_cur_prio() - base_prio == 0);
	}else{
		_TEST(_mutex_pair == NULL);
	}


	return NULL;
}

static void* mutex_unlock_test(void *_mutex){

	/* try to unlock a mutex not owned by this thread (handle must be passed to us by the caller) */
	_TEST(lib_thread__mutex_unlock((mutex_hdl_t*)_mutex) == -ESTD_PERM);


	return NULL;
}

static void* signal_send(void *_arg){

	_TEST(lib_thread__msleep(99) == EOK);
	_TEST(lib_thread__signal_send(_arg) == EOK);
	_TEST(pthread_cond_signal(&(*(signal_hdl_t *)_arg)->sig_cnd) == 0); // simulate "spurious wakeup"
	_TEST(lib_thread__msleep(99) == EOK);
	_TEST(pthread_cond_signal(&(*(signal_hdl_t *)_arg)->sig_cnd) == 0); // simulate "spurious wakeup"
	_TEST(lib_thread__signal_send(_arg) == EOK);
	_TEST(lib_thread__msleep(99) == EOK);
	_TEST(lib_thread__signal_send(_arg) == EOK);
	_TEST(lib_thread__msleep(99) == EOK);
	_TEST(pthread_cond_signal(&(*(signal_hdl_t *)_arg)->sig_cnd) == 0); // simulate "spurious wakeup"
	_TEST(lib_thread__signal_send(_arg) == EOK);
	_TEST(lib_thread__msleep(99) == EOK);
	_TEST(lib_thread__signal_send(_arg) == EOK);


	return NULL;
}

static void* signal_wait(void *_arg){

	_TEST(lib_thread__signal_wait(_arg) == EOK);


	return NULL;
}

static void* signal_timedwait(void *_arg){

	_TEST(lib_thread__signal_timedwait(_arg, 10000) == EOK);


	return NULL;
}

static void* signal_destroy(void *_arg){

	_TEST(lib_thread__msleep(99) == EOK);
	_TEST(lib_thread__signal_destroy(_arg) == EOK);


	return NULL;
}

static void* condvar_wait(void *_arg){
	_TEST(lib_thread__mutex_lock(((struct condvar_wait_arg*)_arg)->mtx) == EOK);
	_TEST(lib_thread__cond_wait(((struct condvar_wait_arg*)_arg)->cond, ((struct condvar_wait_arg*)_arg)->mtx) == EOK);
	_TEST(lib_thread__cond_timedwait(((struct condvar_wait_arg*)_arg)->cond, ((struct condvar_wait_arg*)_arg)->mtx, 1000) == EOK);
	_TEST(lib_thread__cond_timedwait(((struct condvar_wait_arg*)_arg)->cond, ((struct condvar_wait_arg*)_arg)->mtx, 1000) == -EEXEC_TO);
	_TEST(lib_thread__msleep(10000) == EOK);

//	_TEST(lib_thread__cond_signal(((struct condvar_wait_arg*)_arg)->cond) == EOK);						/* post signal to simulate it is in vain */
	_TEST(lib_thread__cond_timedwait(((struct condvar_wait_arg*)_arg)->cond, ((struct condvar_wait_arg*)_arg)->mtx, 1000) == -EEXEC_TO);	/* cond_timedwait: must timeout since signal shall be in vain */

	_TEST(lib_thread__mutex_unlock(((struct condvar_wait_arg*)_arg)->mtx) == EOK);
	return NULL;
}

static void* condvar_thread_lockandsignal_sleep(void *_arg)
{
	_TEST(lib_thread__mutex_lock(((struct condvar_wait_arg*)_arg)->mtx) == EOK);
	_TEST(lib_thread__cond_signal(((struct condvar_wait_arg*)_arg)->cond) == EOK);	/* send signal early, not violating timeout condition */
	_TEST(lib_thread__msleep(2000) == EOK);											/* wait longer than timeout condition to check return value != -EEXEC_TO */
	_TEST(lib_thread__mutex_unlock(((struct condvar_wait_arg*)_arg)->mtx) == EOK);
	return NULL;
}

static void* condvar_thread_waitandtimedwait(void *_arg)
{
	int idx = ((struct condvar_wait_arg*)_arg)->prioidx;
	/* cond_wait test case */
	_TEST(lib_thread__mutex_lock(((struct condvar_wait_arg*)_arg)->mtx) == EOK);
	_TEST(lib_thread__cond_wait(((struct condvar_wait_arg*)_arg)->cond, ((struct condvar_wait_arg*)_arg)->mtx) == EOK);
	s_wup[idx]++;
	_TEST(lib_thread__mutex_unlock(((struct condvar_wait_arg*)_arg)->mtx) == EOK);

	/* synchronize to new test */
	_TEST(lib_thread__msleep(10000) == EOK);

	_TEST(lib_thread__mutex_lock(((struct condvar_wait_arg*)_arg)->mtx) == EOK);
	_TEST(lib_thread__cond_timedwait(((struct condvar_wait_arg*)_arg)->cond, ((struct condvar_wait_arg*)_arg)->mtx, 2000) == EOK);
	s_wup[idx]++;
	_TEST(lib_thread__mutex_unlock(((struct condvar_wait_arg*)_arg)->mtx) == EOK);

	return NULL;
}


static void* semaphore_test(void *_arg){

	_TEST(lib_thread__sem_wait(_arg) == EOK);
	_TEST(lib_thread__msleep(99) == EOK);
	_TEST(lib_thread__sem_trywait(_arg) == EOK);
	_TEST(lib_thread__msleep(99) == EOK);
	_TEST(lib_thread__sem_wait(_arg) == EOK);
	_TEST(lib_thread__sem_post(_arg) == EOK);


	return NULL;
}
