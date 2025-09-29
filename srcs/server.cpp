#include <server.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <iostream>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>
#include <vector>
#include <thread>
#include <sstream>
#include <atomic>

Server::Server(FileLogger *logger) : _logger(logger)
{
    Log(ILogger::LogType::INFO, "Starting server...");
    
	struct sockaddr_in	server_addr;
    int					reuseaddr = 1;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_family 		= AF_INET;
	server_addr.sin_port 		= htons(PORT);
    if ((this->_pollfds[0].fd   = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        Log(ILogger::LogType::ERROR, "The Server didn't start (Socket failed)");
        throw Error("Socket");
    }
    if (setsockopt(this->_pollfds->fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
    {
        Log(ILogger::LogType::ERROR, "The Server didn't start (Setsockopt failed)");
        throw Error("Setsockopt");
    }
    if (bind(this->_pollfds->fd, (const struct sockaddr *)&server_addr, sizeof(server_addr))== -1)
    {
        Log(ILogger::LogType::ERROR, "The Server didn't start (Bind failed)");
        throw Error("Bind");
    }    
    if (listen(this->_pollfds->fd, MAX) == -1)
    {
        Log(ILogger::LogType::ERROR, "The Server didn't start (Listen failed)");
        throw Error("Listen");
    }    

	this->_pollfds[0].events		= POLLIN;
	this->_pollfds[0].revents		= 0;
	this->_numfds                   = 1;
	this->_fds                      = 0;
	for (int i = 1; i < MAX; i++)
	{
		this->_pollfds[i].fd			= -1;
		this->_pollfds[i].events		= POLLIN;
		this->_pollfds[i].revents		= 0;
	}
    for (int i = 0;i < MAX;i++)
        this->_buffer[i] = "";
	Log(ILogger::LogType::INFO, "Server started.");
}


void Server::New_connection()
{
    socklen_t   addrlen = sizeof(struct sockaddr_storage);
    struct      sockaddr_in client_addr;
    char        _ip_str[32];
	int fd_new 	= accept(this->_pollfds->fd, (struct sockaddr *)&client_addr, &addrlen);
	inet_ntop(AF_INET, &(client_addr.sin_addr), _ip_str, INET_ADDRSTRLEN);
    std::string str(_ip_str);
    if (fd_new == -1)
    {
		Log(ILogger::LogType::ERROR, "Accept failed for a client from "+ str);
        return ;
    }
    if (this->_numfds == MAX)
    {
        int i;
		for(i = 1; this->_pollfds[i].fd > 0 && i <= MAX; i++)
			;
        if (this->_pollfds[i].fd == -1)
		{
			this->_pollfds[i].fd 		= fd_new;
			this->_pollfds[i].events 	= POLLIN;
			this->_pollfds[i].revents 	= 0;
			this->_ip[i]         	    = str;
        }
    }
    else if (this->_numfds < MAX)
    {
        this->_pollfds[this->_numfds].fd 	    = fd_new;
		this->_pollfds[this->_numfds].events    = POLLIN;
		this->_pollfds[this->_numfds].revents   = 0;
		this->_ip[this->_numfds]                = str;
		this->_numfds++;
    }
    else
    {
        send(fd_new, "Too much client try again later\n", 32, 0);
        Log(ILogger::LogType::INFO, "User from " + str + " tried to connect but the server is full");
        return ;
    }
    send(fd_new, "MattDaemon~>", 12, 0);
    Log(ILogger::LogType::INFO, "New connection from " + str);
}

void Server::Bash_command(std::string command, int fd)
{
	if (!command.find("shell") || !command.find("/bin/sh") || !command.find("bash") || !command.find("zsh"))
	{
		std::string tmp = "Command blocked\nYou can't execute this command\n";
		send(this->_pollfds[fd].fd, tmp.c_str(), tmp.length(), 0);
		return ;
	}
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stderr = dup(STDERR_FILENO);
    int file = open(".tmp.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file < 0)
    {
		Log(ILogger::LogType::WARNING, "Open Failed, the command " + command + " wasn't executed");
        perror("open");
        return ;
    }
    dup2(file, STDOUT_FILENO);
    dup2(file, STDERR_FILENO);
    close(file);
    system(command.c_str());
    Log(ILogger::LogType::INFO, "The user from " + this->_ip[fd] + " has executed this command '" + command + "' on the server");
    dup2(saved_stderr, STDERR_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    close(saved_stderr);
    std::ifstream fichier(".tmp.txt");
    if (!fichier.is_open())
    {
		Log(ILogger::LogType::WARNING, "Open Failed, the result of the command '" + command + "' hasn't been send");
        return ;
    }
    std::string ligne;
    while (std::getline(fichier, ligne))
    {
        send(this->_pollfds[fd].fd, ligne.c_str(), ligne.length(), 0);
        send(this->_pollfds[fd].fd, "\n", 1, 0);
    }
    fichier.close();
    system("rm .tmp.txt");
}

std::vector<std::string> Server::split(const std::string& command)
{
    std::vector<std::string> tokens;
    std::istringstream iss(command);
    std::string token;
    
    while (iss >> token)
        tokens.push_back(token);
    
    return tokens;
}

int Server::Handle(std::string action, int fd)
{
    std::string tmp;
	std::vector<std::string> command = split(action);

	if (command.empty())
		return 1; 
    if (!command[0].compare("help"))
    {
        tmp = "All The command available on the server:\n\
		help\t\t- display this panel with all the commands\n\
		shell\t\t- execute a command on the server\n\
		shutdown\t- shutdown the taskmaster server\n";
        send(this->_pollfds[fd].fd, tmp.c_str(), tmp.length(), 0);
    }
    else if (!command[0].compare("shell") && command.size() > 1)
	{
		std::string shell_command;
		for (int i = 1; i < command.size(); i++)
			shell_command += command[i] + " ";
        Bash_command(shell_command, fd);
	}
    else if (!command[0].compare("quit"))
    {
        Log(ILogger::LogType::INFO, "The user from " + this->_ip[fd] + " has disconnected");
        close(this->_pollfds[fd].fd);
        this->_pollfds[fd].fd = -1;
        this->_ip[fd] = "";
    }
    else if (!command[0].compare("shutdown"))
    {
        Log(ILogger::LogType::INFO, "The user from " + this->_ip[fd] + " has shutdowned the server");
		for (int i = 0;i < MAX; i++)
			if (this->_pollfds[i].fd != -1)
				close(this->_pollfds[i].fd);
        tmp = "Goodbye\n";
        send(this->_pollfds[fd].fd, tmp.c_str(), tmp.length(), 0);
        return 0;
    }
    else
    {
        action.pop_back();
        Log(ILogger::LogType::INFO, "The user from " + this->_ip[fd] + " input " + action);
        send(this->_pollfds[fd].fd, tmp.c_str(), tmp.length(), 0);
    }
    this->_buffer[fd] = "";
    return 1;
}

int Server::Action(int fd)
{
    char                recv_msg[SIZE_MSG];
	memset(recv_msg, 0, SIZE_MSG);
	ssize_t	numbytes = recv(this->_pollfds[fd].fd, &recv_msg, SIZE_MSG, 0);
    int ok = 1;
	if (numbytes == -1)
		Log(ILogger::LogType::WARNING, "Recv Failed, next command may be incomplete !");
	else if (numbytes == 0)
    {
        Log(ILogger::LogType::INFO, "The user from " + this->_ip[fd] + " has disconnected");
        close(this->_pollfds[fd].fd);
	    this->_pollfds[fd].fd = -1;
        this->_ip[fd] = "";
    }
    else
    {
        std::string str = recv_msg;
		if (str[str.length() - 1] != '\n')
			_buffer[fd] += str;
        else
            ok = Handle(_buffer[fd] + str, fd);
    }
    if (ok)
        send(this->_pollfds[fd].fd, "MattDaemon~>", 12, 0);
    return ok;
}

void	Server::Log(const ILogger::LogType &log_type, const std::string &msg)
{
    this->_logger->Log(log_type, msg);
}

void    Server::Run(std::atomic<bool> &g_running)
{
    while(g_running)
	{
		this->_fds = this->_numfds;
		if (poll(this->_pollfds, this->_fds, -1) == -1)
			Log(ILogger::LogType::WARNING, "Poll Failed an action may have been ignored !");
        else
        {
            for (nfds_t fd = 0; fd <= this->_fds; fd++)
            {
                if (this->_pollfds[fd].fd <= 0)
				    continue;
                if ((this->_pollfds[fd].revents & POLLIN) == POLLIN)
                {
                    if (this->_pollfds[fd].fd == this->_pollfds->fd)
					    New_connection();
                    else
					    if (!Action(fd))
                            return;
                }
            }
        }
	}
}

Server::~Server()
{
    Log(ILogger::LogType::INFO, "Closing server...");
	this->_shutdown = true;
    close(this->_pollfds->fd);
	Log(ILogger::LogType::INFO, "Server closed.");
	system("rm /var/lock/matt_daemon.lock");
}

Server::Error::Error(std::string msg) : msg(msg)
{}

const char* Server::Error::what() const throw()
{return this->msg.c_str();}