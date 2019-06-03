//! this class is a utility class used for connecting remote client and execute some commands
//! Those functions don't check the valiation of the order. So please ensure that the order is valid by youself

#ifndef SSH_UTILITY_H
#define SSH_UTILITY_H

#include<libssh/libssh.h>
#include<iostream>
#include<string.h>

class ssh_utility
{
public:
    static void ExecuteRemoteOrder(std::string net_address, std::string username, std::string password, std::string order);
private:
    static int ExecuteRemoteOrder_(ssh_session session, std::string order);

};


#endif // !SSH_UTILITY_H


