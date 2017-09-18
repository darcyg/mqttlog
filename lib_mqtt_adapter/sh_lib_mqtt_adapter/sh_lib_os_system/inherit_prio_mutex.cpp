#include "inherit_prio_mutex.h"

namespace os_system{

inherit_prio_mutex::inherit_prio_mutex()
{
	// Destroy the underlying mutex
    ::pthread_mutex_destroy(native_handle());

    // Create mutex attribute with desired protocol
    ::pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    // Initialize the underlying mutex
    ::pthread_mutex_init(native_handle(), &attr);
    // The attribute shouldn't be needed any more
    ::pthread_mutexattr_destroy(&attr);
}

}
