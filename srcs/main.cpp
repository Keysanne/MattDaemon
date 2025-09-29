#include <server.h>
#include <iostream>
#include <unistd.h>
#include <atomic>
# include <csignal>
#include <fcntl.h>
#include <filesystem>

FileLogger *g_logger;
std::atomic<bool> g_running(true);
Server *g_server;

void handle_signal(int sig) {
    switch (sig) {
        case SIGINT:
        	g_logger->Log(ILogger::LogType::INFO, "SIGINT received");
            g_running = false;
			break;
        case SIGTERM:
        	g_logger->Log(ILogger::LogType::INFO, "SIGTERM received");
            g_running = false;
			break;
		case SIGQUIT:
        	g_logger->Log(ILogger::LogType::INFO, "SIGQUIT received");
            g_running = false;
            break;
		default:
			g_logger->Log(ILogger::LogType::INFO, "unmanaged signal (" + std::to_string(sig) + ") received");
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
	if (geteuid() != 0)
	{
        std::cout << "You need to be root !" << std::endl;
        return 1;
    }
	if (std::filesystem::exists("/var/lock/matt_daemon.lock"))
	{
		std::cout << "MattDaemon already launched (/var/lock/matt_daemon.lock exist !)" << std::endl;
        return 1;
	}
	g_logger = new FileLogger("/var/log/matt_daemon", "matt_daemon.log");
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
	int fd = open("/dev/null", O_RDWR);
	if (fd != -1)
	{
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
			if (fd > 2)
				close(fd);
	}
	
	setup_signal_handlers();

	try
	{
		system("touch /var/lock/matt_daemon.lock");
		g_server = new Server(g_logger);
		g_server->Run(g_running);
		delete g_server;
	}
	catch (std::exception &e)
	{
		std::cout << "The Server didn't start ("<< e.what() << " failed)" << std::endl;
	}
	return 0;
}