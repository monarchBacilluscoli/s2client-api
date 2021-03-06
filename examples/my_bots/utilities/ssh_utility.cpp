#include "ssh_connection.h"

void SSHConnection::SetConnection(const std::string& net_address, const std::string& username, const std::string& password)
{
	ssh_options_set(m_session, SSH_OPTIONS_HOST, net_address.c_str());
	m_username = username;
	ssh_options_set(m_session, SSH_OPTIONS_USER, m_username.c_str());
	m_password = password;
}

void SSHConnection::Connect()
{
	int rc = ssh_connect(m_session);
	if (rc != SSH_OK) {
		std::cerr << ssh_get_error(m_session);
	}
	ssh_userauth_password(m_session, m_username.c_str(), m_password.c_str());
}

int SSHConnection::Execute(const std::string& order)
{
	ssh_channel channel;
	int rc;
	
	// new channel
	channel = ssh_channel_new(m_session);
	if (channel == NULL) {
		return SSH_ERROR;
	}
	// attach channel
	rc = ssh_channel_open_session(channel);
	if (rc != SSH_OK)
	{
		ssh_channel_free(channel);
		return rc;
	}
	// request execution
	rc = ssh_channel_request_exec(channel, order.c_str());
	if (rc != SSH_OK) {
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return rc;
	}
	// finish the request, kill the used channel
	ssh_channel_send_eof(channel);
	ssh_channel_close(channel);
	ssh_channel_free(channel);
	return SSH_OK;
}
