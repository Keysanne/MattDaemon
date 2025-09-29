#ifndef LOG_H
# define LOG_H

# include <string>
# include <fstream>
# include <exception>
# include <mutex>
# include <memory>

class ILogger
{
	public:
		enum LogType
		{
			INFO,
			WARNING,
			ERROR
		};

		enum Type
		{
			FILE,
			EMAIL,
			SYSLOG
		};

	protected:
		const Type	_type;

	public:
		ILogger(const Type &type) : _type(type) {}
		virtual ~ILogger() = default;
		virtual void Log(const LogType &log_type, const std::string &msg) = 0;

		static std::string	GetDate(void);
		static std::string	GetTime(void);
		static std::string	LogTypeToStr(const LogType &log_type);

		Type	GetType(void) { return this->_type; }
		
};

class FileLogger : public ILogger
{
	private:
		static std::mutex	_mutex;

		std::string			_directory;
		std::string			_file_name;
		std::string			_date;
		std::ofstream		_file;

		void	SetFilePath(void);

	public:
		FileLogger(const std::string &directory, const std::string &file_name);
		FileLogger(const FileLogger &o);
		~FileLogger(void);

		FileLogger &operator =(const FileLogger &o);


		void Log(const LogType &log_type, const std::string &msg);

		class Exception : public std::exception
		{
			private:
				const std::string _message;

			public:
				Exception(const std::string message) : _message(message) {}
				const char *what(void) { return this->_message.c_str(); }
		};
};

class SysLogger: public ILogger
{
	private:

	public:
		SysLogger();
		~SysLogger();
		
		SysLogger &operator =(const SysLogger &o);
		void Log(const LogType &log_type, const std::string &msg);

		class Exception : public std::exception
		{
			private:
				const std::string _message;

			public:
				Exception(const std::string message) : _message(message) {}
				const char *what(void) { return this->_message.c_str(); }
		};
};

bool CreateDirectory(const std::string &path);

#endif