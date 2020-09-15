#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <thread>
#include <iostream>

#include "sc2utils/port_checker.h"

PortChecker::PortChecker()
{
    m_address.sin_family = AF_INET;
    m_address.sin_addr.s_addr = INADDR_ANY; // 因为将IP绑定到本机，所以使用了INADDR_ANY(0.0.0.0)指定了IP
}

bool PortChecker::Check(int port)
{
    if ((m_checker_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // IPv4，可靠连接，这种组合的第0个协议（TCP）
    {
        perror("socket: ");
    }
    m_address.sin_port = htons(port); // 将unsigned short integer hostshort from host byte order to network byte order 即从本机可能是小端的字节序转换成网络字节序（大端）

    if (bind(m_checker_fd, (sockaddr *)&m_address, sizeof(m_address)) < 0) // 将创建的socket绑定在对应的地址和端口
    {
        std::string error_s = "bind(" + std::to_string(port) + ")";
        perror(error_s.c_str());
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

uint16_t PortChecker::GetContinuousPortsFromPort(uint16_t port_start, int continuous_port_num)
{
    for (int i = 0; i < continuous_port_num; ++i) //得到连续7个port
    {
        if (!Check(port_start + i))
        {
            port_start = port_start + i + 1;
            i = -1;
        }
        else if (i < 7 && port_start + i > std::numeric_limits<u_int16_t>::max())
        {
            throw("no ports valid below port number " + std::to_string(std::numeric_limits<u_int16_t>::max()));
        }
    }
    return port_start;
}