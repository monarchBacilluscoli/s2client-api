#include "ssh_utility.h"

void ssh_utility::ExecuteRemoteOrder(std::string net_address, std::string username, std::string password, std::string order)
{
	// Connect to the remote client
	ssh_session session = ssh_new();
	ssh_options_set(session, ssh_options_e::SSH_OPTIONS_HOST, net_address.c_str());
	ssh_options_set(session, SSH_OPTIONS_USER, username.c_str()); //! Here is not obvious, but note that all the parameters are passed by pointers.
	int rc = ssh_connect(session); // Maybe "rc" means response code
	if (rc != SSH_OK) {
		std::cerr << ssh_get_error(session);
		exit(-1);
	}
	rc = ssh_userauth_password(session, username.c_str(), password.c_str());
	if (rc != SSH_AUTH_SUCCESS)
	{
		fprintf(stderr, "Error authenticating with password: %s\n",
			ssh_get_error(session));
		std::cerr << "Error authenticating with password: %s\n" << ssh_get_error(session);
		ssh_disconnect(session);
		ssh_free(session);
		exit(-1);
	}
	//todo launch
	ExecuteRemoteOrder_(session, order);

	ssh_disconnect(session);
	ssh_free(session); //! Note that you should follow the allocate-it-deallocate-it pattern for each object created by xxx_new()

	// 
}

int ssh_utility::ExecuteRemoteOrder_(ssh_session session, std::string order)
{
	ssh_channel channel;
	int rc;
	char buffer[256];
	int nbytes;
	channel = ssh_channel_new(session);
	if (channel == NULL)
		return SSH_ERROR;
	rc = ssh_channel_open_session(channel);
	if (rc != SSH_OK)
	{
		ssh_channel_free(channel);
		return rc;
	}
	//! "nohup" means that the order will not come to an end when the channel is closed
	std::string command = "nohup" + order;
	rc = ssh_channel_request_exec(channel, command.c_str());
	if (rc != SSH_OK)
	{
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return rc;
	}
	nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
	while (nbytes > 0)
	{
		//if (write(1, buffer, nbytes) != (unsigned int)nbytes)
		//{
		//	ssh_channel_close(channel);
		//	ssh_channel_free(channel);
		//	return SSH_ERROR;
		//}
		std::cout << buffer;
		// read the next batch of information
		nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
	}
	if (nbytes < 0)
	{
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return SSH_ERROR;
	}
	ssh_channel_send_eof(channel);
	ssh_channel_close(channel);
	ssh_channel_free(channel);
	return SSH_OK;

}
