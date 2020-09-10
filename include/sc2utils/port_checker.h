#include <netinet/in.h>

class PortChecker
{
private:
    int m_checker_fd;
    int m_opt = 1;
    sockaddr_in m_address;

public:
    PortChecker();
    ~PortChecker();

    bool Check(int port);
    uint16_t GetContinuousPortsFromPort(uint16_t port_start, int continuous_port_num); // check if num continuous ports from port_start valid. If so, return port_start; if not, return the nearest port from port_start from which num continuous ports are valid
};
