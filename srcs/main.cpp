#include <server.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <atomic>

std::atomic<bool> g_running(true);
Server *g_server;

int main(int argc, char **argv, char **env)
{
	if (argc != 2)
	{
		std::cout << "Error\nUsage : ./MattDaemon [config_file]" << std::endl;
		return -1;
	}

	std::cout << "Your server will be on the adress : " << std::endl;
	system("ifconfig | grep -w 'inet 10.*.*.*' | awk '{print $2}' | tr -d '\\n'");
    std::cout << ":" << PORT << std::endl;
	
	pid_t pid, sid;
    pid = fork();
    if (pid < 0)
	{
		std::cout << "The Server didn't start (Fork failed)" << std::endl;
		return -1;
	}	
    if (pid > 0)
	{
		std::cout << "The Server successfully started" << std::endl;
        return 0;
	}
    sid = setsid();
    if (sid < 0)
	{
		std::cout << "The Server didn't start (Setsid failed)" << std::endl;
		return -1;
	}
    close(STDIN_FILENO);
    close(STDERR_FILENO);
    close(STDOUT_FILENO);

	try
	{
		g_server = new Server(argv[1]);
		g_server->Run(g_running);
		delete g_server;
	}
	catch (std::exception &e)
	{
		std::cout << "The Server didn't start ("<< e.what() << " failed)" << std::endl;
	}
	return 0;
}