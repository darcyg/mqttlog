#include <mutex>

namespace os_system {

class inherit_prio_mutex : public std::mutex
{
public:
    inherit_prio_mutex();
};

}
