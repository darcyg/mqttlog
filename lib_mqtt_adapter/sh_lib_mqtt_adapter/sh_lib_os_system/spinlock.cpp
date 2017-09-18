#include "spinlock.h"


namespace os_system {

spinlock::spinlock(): locked(ATOMIC_FLAG_INIT) {}

spinlock::~spinlock() {}

void spinlock::lock()
{
	while (locked.test_and_set(std::memory_order_acquire)) { ; }
}

void spinlock::unlock()
{
	locked.clear(std::memory_order_release);
}


}
