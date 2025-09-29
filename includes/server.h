#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <Logger.h>
# include <sys/poll.h>
# include <exception>
# include <map>
# include <atomic>

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
        const std::string								_config_path;
        std::map<std::string, Job>						_jobs;
        std::map<std::string, std::unique_ptr<ILogger>>	_loggers;
		pthread_t										_check_auto_restart;
		bool											_shutdown = false;

        void    New_connection();
        int     Action(int fd);
        int     Handle(std::string action, int fd);
        void    Bash_command(std::string command, int fd);
        void    Start_service(std::string service_name, int fd);
        bool	UpdateConfig(int fd);
        void    Status(int fd);
        void    Stop_service(std::string service_name, int fd);
		void	Restart_service(std::string service_name, int fd);
		static void	*Routine_Check_restart(void *data);
		
		public:
        Server(const std::string &config_path);
        ~Server();
        void    Run(std::atomic<bool> &g_running);
		void	Reload_config(int fd);
        void	Log(const ILogger::LogType &log_type, const std::string &msg);
		std::map<std::string, Job> *GetJobs(void) {return &this->_jobs;}
		bool GetShutdown(void) {return this->_shutdown;}

        static std::vector<std::string> split(const std::string& command);
        
        class Error: public std::exception
		{
            private:
                std::string msg;
            public:
                Error(std::string);    
                const char* what() const throw();
        };
    friend Job;
};

#endif