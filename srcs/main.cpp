#include <server.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <atomic>
# include <csignal>
#include <Logger.h>

FileLogger g_logger("/var/log/matt_daemon", "matt_daemon.log");
std::atomic<bool> g_running(true);
Server *g_server;

void handle_signal(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
		case SIGQUIT:
            g_running = false;
            break;
    }
}

void setup_signal_handlers() {
    struct sigaction sa {};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP,  &sa, nullptr);
}

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
	
	setup_signal_handlers();

	try
	{
		g_server = new Server(&g_logger);
		g_server->Run(g_running);
		delete g_server;
	}
	catch (std::exception &e)
	{
		std::cout << "The Server didn't start ("<< e.what() << " failed)" << std::endl;
	}
	return 0;
}