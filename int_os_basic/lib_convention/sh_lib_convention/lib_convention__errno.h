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
#include <errno.h>

/* *******************************************************************
 * defines
 * ******************************************************************/
#ifndef NULL
  #define NULL      0
#endif

/* *******************************************************************
 * >>>>>	unhandy fault										<<<<<<
 * ******************************************************************/
#define ESTD_PERM		1001	/* Operation not permitted */
#define	ESTD_NOFILE		1002	/* No such file or directory */
#define	ESTD_NOENT		1002
#define	ESTD_SRCH		1003	/* No such process */
#define	ESTD_INTR		1004	/* Interrupted function call */
#define	ESTD_IO			1005	/* Input/output error */
#define	ESTD_NXIO		1006	/* No such device or address */
#define	ESTD_2BIG		1007	/* Arg list too long */
#define	ESTD_NOEXEC		1008	/* Exec format error */
#define	ESTD_BADF		1009	/* Bad file descriptor */
#define	ESTD_CHILD		1010	/* No child processes */
#define	ESTD_AGAIN		1011	/* Resource temporarily unavailable */
#define	ESTD_NOMEM		1012	/* Not enough space */
#define	ESTD_ACCES		1013	/* Permission denied */
#define	ESTD_FAULT		1014	/* Bad address */
#define	ESTD_BUSY		1016	/* strerror reports "Resource device" */
#define	ESTD_EXIST		1017	/* File exists */
#define	ESTD_XDEV		1018	/* Improper link (cross-device link?) */
#define	ESTD_NODEV		1019	/* No such device */
#define	ESTD_NOTDIR		1020	/* Not a directory */
#define	ESTD_ISDIR		1021	/* Is a directory */
#define	ESTD_INVAL		1022	/* Invalid argument */
#define	ESTD_NFILE		1023	/* Too many open files in system */
#define	ESTD_MFILE		1024	/* Too many open files */
#define	ESTD_NOTTY		1025	/* Inappropriate I/O control operation */
#define	ESTD_FBIG		1027	/* File too large */
#define	ESTD_NOSPC		1028	/* No space left on device */
#define	ESTD_SPIPE		1029	/* Invalid seek (seek on a pipe?) */
#define	ESTD_ROFS		1030	/* Read-only file system */
#define	ESTD_MLINK		1031	/* Too many links */
#define	ESTD_PIPE		1032	/* Broken pipe */
#define	ESTD_DOM		1033	/* Domain error (math functions) */
#define	ESTD_RANGE		1034	/* Result too large (possibly too small) */

/* Parameter error (validity, range and format, e.g. EINVAL, EBADF, E2BIG) ****/
#define	EPAR_NULL		1100	/* NULL pointer passed as argument		*/
#define	EPAR_RANGE		1101	/* argument value out of allowed range	*/
#define EPAR_COMBI		1102	/* argument value not supported by this configuration */
#define EPAR_INVCHN		1103	/* channel invalid */
#define EPAR_INVCONFIG	1104	/* configuration invalid */
#define EPAR_NOCONFIG	1105	/* parameter not configured*/
#define EPAR_INVIOTYPE	1106	/* invalid IO type */
#define EPAR_INVCHTYPE	1107	/* invalid channel type */
#define	EPAR_INVVALUEID 1108	/* invalid channel type */
#define	EPAR_INVCONFIGTYPE 1109	/* invalid channel type */
#define EPAR_OPNOTSUPP	1110	/* parameter operation not supported */
#define EPAR_BADVALUE	1119	/* unexpected or invalid value */

/* Execution error (timing and results, e.g. EINTR, EBUSY, ENOEXEC) ***********/
#define	EEXEC_TO		1200	/* execution timed out					*/
#define	EEXEC_NOINIT	1201	/* component not (yet) initialized		*/
#define EEXEC_AGAININIT	1202	/* component already (still) initialized	*/
#define EEXEC_FAILINIT	1203	/* initialization of component failed */
#define EEXEC_OPNOTSUPP 1204	/* operation not supported */
#define EEXEC_DEADLK 	1205	/* deadlock */
#define EEXEC_CLEANUP	1206

/* Permission error (create, modify or delete, e.g. ENOENT, EEXIST, EACCESS) **/
#define	EPERM_RO		1600	/* resource is write-protected			*/
#define	EPERM_WO		1601	/* resource is read-protected			*/

/* Communication error (network or IPC, e.g. EXDEV, EPIPE, ECONNRESET) ********/
#define	ECOMM_CRC		1700	/* CRC error on data transmission		*/
#define	ECOMM_TO		1701	/* communication timeout				*/
#define ECOMM_CONDENIED	1702	/* connection request denied			*/
#define ECOMM_ALRDYCON	1703	/* connection already active			*/
#define ECOMM_NOCON		1704	/* connection not active				*/
#define ECOMM_BADLENGTH	1705	/* message length wrong					*/
#define ECOMM_BADREQ	1706	/* message request wrong				*/
#define ECOMM_BADCONTENT 1707	/* message request wrong*/

#define EHAL_ERROR		1800	/* HAL Error */
#define EOK				0
/* *******************************************************************
 * static inline function
 * ******************************************************************/

/* ************************************************************************//**
 * \brief	Convert routine form standard to custom values
 *
 * \param	_error_code		Error Code to convert
 * \return	return converted MAN errno code, or negative standard error
 * 			if not exits.
 * ****************************************************************************/
static inline int convert_std_errno(int _error_code)
{
    switch (_error_code)
    {
        /* "standard" error codes */
        case EPERM	:	       	return -ESTD_PERM	;
        case ENOENT	:	       	return -ESTD_NOENT	;
        case ESRCH	:	       	return -ESTD_SRCH	;
        case EINTR	:	       	return -ESTD_INTR	;
        case EIO	:	       	return -ESTD_IO		;
        case ENXIO	:	       	return -ESTD_NXIO	;
        case E2BIG	:	       	return -ESTD_2BIG	;
        case ENOEXEC:	       	return -ESTD_NOEXEC	;
        case EBADF	:	       	return -ESTD_BADF	;
        case ECHILD	:	       	return -ESTD_CHILD	;
        case EAGAIN	:	       	return -ESTD_AGAIN	;
        case ENOMEM	:	       	return -ESTD_NOMEM	;
        case EACCES	:	       	return -ESTD_ACCES	;
        case EFAULT	:	       	return -ESTD_FAULT	;
        case EBUSY	:	       	return -ESTD_BUSY	;
        case EEXIST	:	       	return -ESTD_EXIST	;
        case EXDEV	:	       	return -ESTD_XDEV	;
        case ENODEV	:	       	return -ESTD_NODEV	;
        case ENOTDIR:           return -ESTD_NOTDIR	;
        case EISDIR	:	       	return -ESTD_ISDIR	;
        case EINVAL	:	       	return -ESTD_INVAL	;
        case ENFILE	:	       	return -ESTD_NFILE	;
        case EMFILE	:	       	return -ESTD_MFILE	;
        case ENOTTY	:	       	return -ESTD_NOTTY	;
        case EFBIG	:	       	return -ESTD_FBIG	;
        case ENOSPC	:	       	return -ESTD_NOSPC	;
        case ESPIPE	:	       	return -ESTD_SPIPE	;
        case EROFS	:	       	return -ESTD_ROFS	;
        case EMLINK	:	       	return -ESTD_MLINK	;
        case EPIPE	:	       	return -ESTD_PIPE	;
        case EDOM	:	       	return -ESTD_DOM	;
        case ERANGE	:	       	return -ESTD_RANGE	;
        case EDEADLK:			return -EEXEC_DEADLK;
        case ETIMEDOUT: 		return -EEXEC_TO    ;


        /* epoll_ctl() */
        case ELOOP:		       	return -EPAR_COMBI;		// fd refers to an epoll instance and this EPOLL_CTL_ADD operation would result in a circular loop of epoll instances monitoring one another.

        /* socket() */
        case EAFNOSUPPORT:		return -EPAR_INVCHTYPE;		// The implementation does not support the specified address family.
        case ENOBUFS:			return -ESTD_NOMEM;		// Insufficient memory is available. The socket cannot be created until sufficient resources are freed.
        case EPROTONOSUPPORT:	return -EPAR_INVCHN;		// The protocol type or the specified protocol is not supported within this domain.

        /* bind() */
        case EADDRINUSE:		return -EPAR_OPNOTSUPP;		// The given address is already in use.
        case ENOTSOCK:			return -EPAR_INVCONFIGTYPE;	// The file descriptor sockfd does not refer to a socket.
        case EADDRNOTAVAIL:		return -EPAR_INVCONFIG;		// A nonexistent interface was requested or the requested address was not local.
//	case ELOOP:				return -EPAR_COMBI;			// Too many symbolic links were encountered in resolving addr.
        case ENAMETOOLONG:		return -EPAR_BADVALUE;		// addr is too long.

        /* sendto() */
        case ECONNRESET:		return -ECOMM_CONDENIED;	// Connection reset by peer.
        case EDESTADDRREQ:		return -ECOMM_BADREQ;		// The socket is not connection-mode, and no peer address is set.
        case EISCONN:			return -ECOMM_ALRDYCON;		// The connection-mode socket was connected already but a recipient was specified. (Now either this error is returned, or the recipient specification is ignored.)
        case EMSGSIZE:			return -ECOMM_BADLENGTH;	// The socket type requires that message be sent atomically, and the size of the message to be sent made this impossible.
//	case ENOBUFS:			return -ESTD_NOMEM;			// The output queue for a network interface was full. This generally indicates that the interface has stopped sending, but may be caused by transient congestion. (Normally, this does not occur in Linux. Packets are just silently dropped when a device queue overflows.)
        case ENOTCONN:			return -ECOMM_NOCON;		// The socket is not connected, and no target has been given.
//	case ENOTSOCK:			return -EPAR_INVIOTYPE;		// The file descriptor sockfd does not refer to a socket.
        case EOPNOTSUPP:		return -EEXEC_OPNOTSUPP;	// Some bit in the flags argument is inappropriate for the socket type.

        /* recvmmsg() */
        case ECONNREFUSED:		return -ECOMM_CONDENIED;	// A remote host refused to allow the network connection (typically because it is not running the requested service).
//	case ENOTCONN:			return -ECOMM_NOCON;		// The socket is associated with a connection-oriented protocol and has not been connected (see connect(2) and accept(2)).
//	case ENOTSOCK:			return -EPAR_INVCONFIGTYPE;	// The file descriptor sockfd does not refer to a socket.

        /* write() */
//	case EDESTADDRREQ:		return -ECOMM_BADREQ;		// The socket is not connection-mode, and no peer address is set.
        case EDQUOT:			return -EPAR_RANGE;			// The user's quota of disk blocks on the filesystem containing the file referred to by fd has been exhausted.

        /* get-/setsockopt() */
        case ENOPROTOOPT:		return -EPAR_INVVALUEID;	// The option is unknown at the level indicated.
//	case ENOTSOCK:			return -EPAR_INVIOTYPE;		// The file descriptor sockfd does not refer to a socket.

        /* open() */
//	case ELOOP:				return -EPAR_COMBI;			// Too many symbolic links were encountered in resolving pathname.
//	case ELOOP:				return -EPAR_COMBI;			// pathname was a symbolic link, and flags specified O_NOFOLLOW but not O_PATH.
//	case EDQUOT:			return -EPAR_RANGE;			// Where O_CREAT is specified, the file does not exist, and the user's quota of disk blocks or inodes on the filesystem has been exhausted.
//	case ENAMETOOLONG:		return -EPAR_BADVALUE;		// pathname was too long.
//	case EOPNOTSUPP:		return -EEXEC_OPNOTSUPP;	// The filesystem containing pathname does not support O_TMPFILE.
        case EOVERFLOW:			return -EPAR_RANGE;			// pathname refers to a regular file that is too large to be opened. The usual scenario here is that an application compiled on a 32-bit platform without -D_FILE_OFFSET_BITS=64 tried to open a file whose size exceeds (1<<31)-1 bytes; see also O_LARGEFILE above. This is the error specified by POSIX.1; in kernels before 2.6.24, Linux gave the error EFBIG for this case.
        case ETXTBSY:			return -EPERM_RO;			// pathname refers to an executable image which is currently being executed and write access was requested.


        /* unknown / unexpected error */
        default:			return -errno;				// return negative errno to indicate missing mapping
    }
}



#endif /* GEN_LIB_CONVENTION__ERRNO_H_ */
