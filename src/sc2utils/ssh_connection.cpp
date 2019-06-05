#include <sc2utils/ssh_connection.h>

namespace sc2 {
	void SSHConnection::SetConnection(const std::string& net_address)
	{
		ssh_options_set(m_session, SSH_OPTIONS_HOST, net_address.c_str());
		m_host_address = net_address;
	}

	std::string SSHConnection::GetHostAddress()
	{
		return m_host_address;
	}

	void SSHConnection::Connect(const std::string& username, const std::string& password)
	{
		int rc = ssh_connect(m_session);
		if (rc != SSH_OK) {
			std::cerr << ssh_get_error(m_session);
		}
		ssh_userauth_password(m_session, username.c_str(), password.c_str());
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
		// request executione
		rc = ssh_channel_request_exec(channel, ("nohup "+order).c_str());
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
		return 0;
	}

}

