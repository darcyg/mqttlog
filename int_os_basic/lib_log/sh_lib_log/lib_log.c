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
 *  21 April 2015			Tom			- add of comments anf logging messages
 *
 */

/* *******************************************************************
 * includes
 * ******************************************************************/

/* c-runtime */
#include <stdlib.h>

#ifdef __unix__
#include <stdio.h>
#elif TRACE
#include <Trace.h>
#else
#error "No output is defined"
#endif

#include <stdarg.h>


/* own libs */

/* project */
#include "lib_log.h"

/* *******************************************************************
 * defines
 * ******************************************************************/

#ifdef __unix__
#define M_WRITE_FORMATED_OUTPUT(_format, ...) 		do { fprintf(stdout, _format, __VA_ARGS__); } while (0)
#define M_WRITE_FORMATED_OUTPUT_VA(_format, _vargs) do { vfprintf(stdout, _format, _vargs); } while (0)
#define M_PRINT_FORMATED_OUTPUT_END					do { fflush(stdout); } while (0)

#else
#define M_WRITE_FORMATED_OUTPUT(_format, ...) 	    do { trace_printf(_format, __VA_ARGS__); } while (0)
#define M_WRITE_FORMATED_OUTPUT_VA(_format, _vargs) do { trace_vprintf(_format, _vargs); } while (0)
#define M_PRINT_FORMATED_OUTPUT_END					do { trace_putchar('\n'); } while (0)

#endif


/* *******************************************************************
 * (static) variables declarations
  ******************************************************************/
static enum log_level s_log_level = LOG_LEVEL_info;

/* *******************************************************************
 * static function declarations
 * ******************************************************************/
static char* lib_log__str_log_level(enum log_level _level);


/* *******************************************************************
 * \brief	Set active logging level
 * ---------
 * \remark
 * ---------
 *
 * \param	_level		: Set of logging level
 *
 * ---------
 * \return	void
 * ******************************************************************/
void lib_log__set_level(enum log_level _level)
{
	s_log_level = _level;
}

/* *******************************************************************
 * \brief	Get active logging level
 * ---------
 * \remark
 * ---------
 *
 * \param	void
 *
 * ---------
 * \return	reqest of active logging level
 * ******************************************************************/
enum log_level lib_log__get_level(void)
{
	return s_log_level;
}

/* *******************************************************************
 * \brief	Print of logging messages
 * ---------
 * \remark
 * ---------
 *
 * \param	_level				: 	Logging message level
 * 			_module_name [in]	:	Module name
 * 			_format [in]		:	Formatted output to print
 *
 * ---------
 * \return	void
 * ******************************************************************/
void lib_log__msg(enum log_level _level, const char *_module_name, const char *_format, ...)
{
	va_list args;
	//char *str1, *str2;


	if (_level < s_log_level)
	    return;

	/* prefix */
	if (_module_name == NULL)
	{

		M_WRITE_FORMATED_OUTPUT("[%s] ",lib_log__str_log_level(_level));
		//fprintf(stdout,"[%s] ",lib_log__str_log_level(_level));
	}
	else
	{
		M_WRITE_FORMATED_OUTPUT("[%s] - %s ",lib_log__str_log_level(_level),_module_name);
	//	fprintf(stdout,"[%s] - %s ",lib_log__str_log_level(_level),_module_name);
	}

	va_start(args, _format);
	M_WRITE_FORMATED_OUTPUT_VA(_format, args);
	//vfprintf(stdout, _format, args);
	va_end(args);

	M_PRINT_FORMATED_OUTPUT_END;


}



static char* lib_log__str_log_level(enum log_level _level)
{
	switch (_level)
	{

		case LOG_LEVEL_critical			: return "CRIT";
		case LOG_LEVEL_error			: return "ERR";
		case LOG_LEVEL_warning			: return "WRN";
		case LOG_LEVEL_info				: return "INFO";
		case LOG_LEVEL_debug_prio_2 	: return "DBG_PRIO2";
		case LOG_LEVEL_debug_prio_1		: return "DBG_PRIO1";
		default					: return "INV";
	}
}

