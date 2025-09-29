#include <Logger.h>
#include <chrono>
#include <ctime>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <iomanip>
#include <syslog.h>

std::mutex FileLogger::_mutex;

std::string ILogger::GetDate(void)
{
	std::ostringstream oss;
	auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm tm = *std::localtime(&t);
	oss << std::put_time(&tm, "%Y-%m-%d");
	return oss.str();
}

std::string ILogger::GetTime(void)
{
	std::ostringstream oss;
	auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm tm = *std::localtime(&t);
	oss << std::put_time(&tm, "%H:%M:%S");
	return oss.str();
}

std::string ILogger::LogTypeToStr(const ILogger::LogType &log_type)
{
	switch (log_type)
	{
		case ILogger::LogType::INFO:
			return "INFO";

		case ILogger::LogType::WARNING:
			return "WARNING";

		case ILogger::LogType::ERROR:
			return "ERROR";
		
		default:
			break;
	}

	return "";
}



FileLogger::FileLogger(const std::string &directory, const std::string &file_name) : ILogger(FILE), _directory(directory), _file_name(file_name)
{
	if (!CreateDirectory(directory))
		throw Exception("FileLogger error: Impossible to create directory '" + directory + "'.");

	this->_date = GetDate();
	SetFilePath();
}

FileLogger::FileLogger(const FileLogger &o) : ILogger(FILE), _directory(o._directory), _file_name(o._file_name), _date(o._date)
{
	SetFilePath();
}

FileLogger::~FileLogger(void)
{
	this->_file.close();
}

FileLogger &FileLogger::operator =(const FileLogger &o)
{
	this->_directory = o._directory;
	this->_file_name = o._file_name;
	this->_date = o._date;
	SetFilePath();
	
	return *this;
}

bool FileLogger::operator ==(const Config::Log &conf)
{
	if ((ILogger::Type)conf.type != ILogger::Type::FILE)
		throw Exception("FileLogger error: Comparison with invalid Log Type");

	if (	this->_directory != conf.directory
		||	this->_file_name != conf.file_name)
		return false;

	return true;
}

void FileLogger::Log(const ILogger::LogType &log_type, const std::string &msg)
{
	this->_mutex.lock();

	std::string date = GetDate();

	if (date != this->_date)
		SetFilePath();

	this->_file << "[" << GetTime() << "] " << ILogger::LogTypeToStr(log_type) << ": " << msg << std::endl;

	this->_mutex.unlock();
}

void FileLogger::SetFilePath(void)
{
	this->_date = GetDate();

	std::string file_path(this->_directory + "/");

	if (this->_file_name == "")
		file_path += this->_date;
	else
	{
		size_t point_pos = this->_file_name.find('.');
		if (point_pos != std::string::npos)
		{
			std::string base = this->_file_name.substr(0, point_pos);
			std::string extention = this->_file_name.substr(point_pos, this->_file_name.length() - point_pos);

			if (base != "")
				base += "_";
			file_path += base + this->_date + extention;
		}
		else
			file_path += this->_file_name + "_" + this->_date;
	}

	if (this->_file.is_open())
		this->_file.close();

	this->_file.open(file_path, std::fstream::app);

	if (!this->_file.is_open())
		throw Exception("FileLogger error: Impossible to open log file '" + file_path + "'.");
}

void FileLogger::Update(const Config::Log &conf)
{
	if (*this == conf)
		return;

	this->_file.close();

	this->_directory = conf.directory;
	this->_file_name = conf.file_name;

	SetFilePath();
}



std::unique_ptr<ILogger> CreateLoggerByConf(const Config::Log &conf)
{
	switch (conf.type)
	{
		case ILogger::Type::FILE:
			return std::unique_ptr<ILogger>(new FileLogger(conf.directory, conf.file_name));
		case ILogger::Type::SYSLOG:
			return std::unique_ptr<ILogger>(new SysLogger());
		default:
			break;
	}

	return std::unique_ptr<ILogger>(new FileLogger(conf.directory, conf.file_name));
}

bool CreateDirectory(const std::string& path)
{
	struct stat info;
	std::string current_path;

	size_t start = 0;
	if (!path.empty() && path[0] == '/') {
		current_path = "/";
		start = 1;
	} else if (path.substr(0, 2) == "./") {
		current_path = ".";
		start = 2;
	} else if (path[0] == '.') {
		current_path = ".";
		start = 1;
	}

	while (start < path.length()) {
		size_t slash_pos = path.find('/', start);
		std::string part = path.substr(start, slash_pos - start);

		if (!part.empty()) {
			if (!current_path.empty() && current_path.back() != '/')
				current_path += "/";
			current_path += part;

			if (stat(current_path.c_str(), &info) != 0) {
				if (mkdir(current_path.c_str(), 0777) == -1 && errno != EEXIST) {
					return false;
				}
			} else if (!(info.st_mode & S_IFDIR)) {
				return false;
			}
		}

		if (slash_pos == std::string::npos)
			break;

		start = slash_pos + 1;
	}

	return true;
}

SysLogger::SysLogger() : ILogger(SYSLOG)
{
	openlog("taskmaster", LOG_PID | LOG_CONS, LOG_USER);
}

SysLogger::~SysLogger()
{
	closelog();
}

SysLogger &SysLogger::operator =(const SysLogger &o)
{
	return *this;
}

bool SysLogger::operator ==(const Config::Log &conf)
{
	if ((ILogger::Type)conf.type != ILogger::Type::SYSLOG)
		throw Exception("SysLogger error: Comparison with invalid Log Type");

	return true;
}

void SysLogger::Log(const LogType &log_type, const std::string &msg)
{
	switch(log_type)
	{
		case LogType::INFO:
			syslog(LOG_INFO, "%s", msg.c_str());
			break;
		case LogType::WARNING:
			syslog(LOG_WARNING, "%s", msg.c_str());
			break;
		case LogType::ERROR:
			syslog(LOG_ERR, "%s", msg.c_str());
			break;
		default:
			return ;
	}
}

void SysLogger::Update(const Config::Log &conf)
{
	if (*this == conf)
		return;
}