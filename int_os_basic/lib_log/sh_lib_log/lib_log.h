/* ****************************************************************************************************
 * lib_log.h within the following project: lib_log
 *
 *  compiler:   GNU Tools ARM LINUX
 *  target:     armv6
 *  author:	    Tom
 * ****************************************************************************************************/

/* ****************************************************************************************************/

/*
 *	******************************* change log *******************************
 *  date			user			comment
 * 	15 April 2015			Tom			- creation of lib_log.h
 *  21 April 2015			Tom			- add of comments
 *
 */


#ifndef SRC_LIB_LOG_H_
#define SRC_LIB_LOG_H_


/* *******************************************************************
 * custom data types (e.g. enumerations, structures, unions)
 * ******************************************************************/
enum log_level
{
	LOG_LEVEL_debug_prio_1,
	LOG_LEVEL_debug_prio_2,
	LOG_LEVEL_info,
	LOG_LEVEL_warning,
	LOG_LEVEL_error,
	LOG_LEVEL_critical
};


/* *******************************************************************
 * defines
 * ******************************************************************/
#ifdef TRACE
	#define msg(_level, _moduel_name, ...) do { \
			lib_log__msg(_level, _moduel_name, __VA_ARGS__);  \
	} while (0)
#else
	#define msg
#endif


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
void lib_log__set_level(enum log_level _level);

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
enum log_level lib_log__get_level(void);

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
void lib_log__msg(enum log_level _level, const char *_module_name, const char *_format, ...);



#endif /* SRC_LIB_LOG_H_ */
