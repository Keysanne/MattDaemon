#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <Logger.h>
# include <sys/poll.h>
# include <exception>
# include <map>
# include <atomic>
# include <vector>

# define MAX 3
# define PORT 4242
# define SIZE_MSG 100

class Server
{
    private:
        int                                             _fds;
        int                                             _numfds;
        std::string                                     _ip[MAX];
        std::string	                                    _buffer[MAX];
        struct	pollfd                                  _pollfds[MAX];
        FileLogger *                                    _logger;
		bool											_shutdown = false;

        void    New_connection();
        int     Action(int fd);
        int     Handle(std::string action, int fd);
        void    Bash_command(std::string command, int fd);
        std::vector<std::string> split(const std::string& command);
		
		public:
        Server(FileLogger *logger);
        ~Server();
        void    Run(std::atomic<bool> &g_running);
        void	Log(const ILogger::LogType &log_type, const std::string &msg);
		bool GetShutdown(void) {return this->_shutdown;}

        class Error: public std::exception
		{
            private:
                std::string msg;
            public:
                Error(std::string);    
                const char* what() const throw();
        };
};

#endif