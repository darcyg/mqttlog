
#include <atomic>



namespace os_system {

class spinlock {
    
public:
	spinlock();
	~spinlock();
    void lock();
    void unlock();
private:
    std::atomic_flag locked;
    
};


}
