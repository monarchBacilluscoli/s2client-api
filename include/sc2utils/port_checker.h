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
};
