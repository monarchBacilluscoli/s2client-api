#include <thread>
#include <iostream>

#include <unistd.h>
#include <sys/socket.h>

#include "sc2utils/port_checker.h"

PortChecker::PortChecker()
{
    m_address.sin_family = AF_INET;
    m_address.sin_addr.s_addr = INADDR_ANY; // 因为将IP绑定到本机，所以使用了INADDR_ANY(0.0.0.0)指定了IP
}

bool PortChecker::Check(int port)
{
    // std::cout << ::getpid() << std::endl;
    if ((m_checker_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // IPv4，可靠连接，这种组合的第0个协议（TCP）
    {
        perror("socket: ");
    }
    if (setsockopt(m_checker_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &m_opt, sizeof(m_opt))) // 设置一些选项，完全可选的。
    {
        perror("setsockopt:");
    }
    m_address.sin_port = htons(port); // 将unsigned short integer hostshort from host byte order to network byte order 即从本机可能是小端的字节序转换成网络字节序（大端）

    if (bind(m_checker_fd, (sockaddr *)&m_address, sizeof(m_address)) < 0) // 将创建的socket绑定在对应的地址和端口
    {
        perror("bind");
        return false;
    }
    else
    {
        if (close(m_checker_fd) < 0) // 关闭file descriptor
        {
            std::cout << "bind ok, release error: " << std::endl;
            perror("shutdown");
        }
        return true;
    }
}

uint16_t PortChecker::GetContinuousPortFromPort(uint16_t port_start, int continuous_port_num)
{
    for (int i = 0; i < continuous_port_num; ++i) //得到连续7个port
    {
        if (!Check(port_start + i))
        {
            port_start = port_start + i + 1;
            i = 0;
        }
        else if (i < 7 && port_start + i > std::numeric_limits<u_int16_t>::max())
        {
            throw("no ports valid below port number " + std::to_string(std::numeric_limits<u_int16_t>::max()));
        }
    }
    return port_start;
}

PortChecker::~PortChecker()
{
    close(m_checker_fd);
}
