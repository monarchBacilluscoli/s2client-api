//! this class is a utility class used for connecting remote client and execute some commands
//! Those functions don't check the valiation of the order. So please ensure that the order is valid by youself

#ifndef SSH_CONNECTION_H
#define SSH_CONNECTION_H

#include<libssh/libssh.h>
#include<iostream>
#include<string.h>

class SSHConnection
{
public:
    SSHConnection() {
        m_session = ssh_new();
    }
    SSHConnection(const std::string& net_address){
        m_session = ssh_new();
        ssh_options_set(m_session, SSH_OPTIONS_HOST, net_address.c_str());
        m_host_address = net_address;
    }
    //! Sets the connection to a remote client
    void SetConnection(const std::string& net_address);
    //! get host address for further use
    std::string GetHostAddress();
    //! Connects to the seted client
    void Connect(const std::string& username, const std::string& password);
    //! check if or not the session is connected
    bool IsConnected() {
        return ssh_is_connected(m_session);
    }
    //! Executes an order without the display of returned data
    int Execute(const std::string& order);
    //!
    ~SSHConnection() {
        ssh_disconnect(m_session);
        ssh_free(m_session);
    }
private:
    ssh_session m_session;
    std::string m_host_address;
};


#endif // !SSH_CONNECTION_H


