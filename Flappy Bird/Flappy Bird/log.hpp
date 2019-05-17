﻿#pragma once

// Exceptions
extern class openFileFailed;

class Outdev
{
public:
	Outdev();

	Outdev(std::wstring &logFilePath);

	~Outdev();

	void init();

	bool init(std::wstring &logFilePath);

	void close();
	bool clear();

	template<class T>
	friend Outdev &operator<<(Outdev &od, T &msg);

private:
	friend class Log;
	bool isInited = false, isOpened = false;
	bool goodbit = false, badbit = false, failbit = false, eofbit = false;
	std::wstring *logFilePath = NULL;
	std::wofstream *logFile = NULL;
};

class Log
{
public:
	class _myendl {} endl;

	Log();
	Log(std::wstring &logFilePath);

	~Log();

	void init();
	bool init(std::wstring &logFilePath);

	void close();
	bool clear();

	template<class T>
	friend Log &operator<<(Log &log, T &msg);
	friend Log &operator<<(Log &log, _myendl &endl);

private:
	Outdev *od = NULL;

	// For thread safety, I have to add a buffer to avoid the thread-unsafe std::ostream and std::wofstream.
	bool isBOL = true;
	HANDLE hThflushLogQueue = NULL;
	std::wstring tmsg;
	std::queue<std::wstring> logQueue;
	static DWORD WINAPI flushLogQueue(LPVOID lpParam);
};



Outdev::Outdev()
{
	init();
}

Outdev::Outdev(std::wstring &logFilePath)
{
	init(logFilePath);
}

Outdev::~Outdev()
{
	close();
}

void Outdev::init()
{
	if (isInited)
		close();

	isInited = true;
	isOpened = true;
	goodbit = true;
	Outdev::logFilePath = NULL;
	Outdev::logFile = NULL;
}

bool Outdev::init(std::wstring &logFilePath)
{
	std::wstring excwstr = L"不能打开文件:";
	if (isInited)
		close();

	Outdev::logFilePath = new std::wstring;
	if (Outdev::logFilePath == NULL)
		throw std::bad_alloc();
	Outdev::logFilePath = &logFilePath;

	isInited = true;

	Outdev::logFile = new std::wofstream;
	if (Outdev::logFilePath == NULL)
		throw std::bad_alloc();

	logFile->open(logFilePath);

	if (!(*logFile))
	{
		throw stdWCexception(excwstr + logFilePath);
		return false;
	}
	else
	{
		isOpened = true;
		goodbit = true;
		return true;
	}
}

bool Outdev::clear()
{
	if (Outdev::isInited)
	{
		std::wcout.clear();

		badbit = failbit = eofbit = false;
		goodbit = true;

		if (Outdev::logFile != NULL)
		{
			logFile->clear();
		}

		return true;
	}
	else
		return false;
}

void Outdev::close()
{
	// Dumping all things out and reset!

	isInited = isOpened = goodbit = badbit = failbit = eofbit = false;

	try
	{
		delete Outdev::logFile;
	}
	catch (...) {}

	try
	{
		delete Outdev::logFilePath;
	}
	catch (...) {}

	Outdev::logFilePath = NULL;
	Outdev::logFile = NULL;
}

template<class T>
Outdev &operator<<(Outdev &od, T &msg)
{
	if (od.isInited && od.isOpened && od.goodbit)
	{
		std::wcout << msg;
		if (od.logFile != NULL)
		{
			*od.logFile << msg;

			od.badbit = std::wcout.bad() || od.logFile->bad();
			od.failbit = std::wcout.fail() || od.logFile->fail();
			od.eofbit = std::wcout.eof() || od.logFile->eof();
			od.goodbit = !(od.badbit || od.failbit || od.eofbit);
		}
		else
		{
			od.badbit = std::wcout.bad();
			od.failbit = std::wcout.fail();
			od.eofbit = std::wcout.eof();
			od.goodbit = !(od.badbit || od.failbit || od.eofbit);
		}
	}
	return od;
}

// Class Log

Log::Log()
{
	init();
}

Log::Log(std::wstring &logFilePath)
{
	init(logFilePath);
}

Log::~Log()
{
	close();
}

void Log::init()
{
	if (od != NULL)
		close();

	od = new Outdev;
	od->init();
	Log::hThflushLogQueue = CreateThread(NULL, 0, Log::flushLogQueue, this, 0, NULL);
}

bool Log::init(std::wstring &logFilePath)
{
	if (od != NULL)
		close();

	od = new Outdev;
	return od->init(logFilePath);
}

void Log::close()
{
	if (od != NULL)
	{
		od->close();
		delete od;
	}
	od = NULL;
}

bool Log::clear()
{
	if (od != NULL)
		return od->clear();
	else
		return false;
}

template<class T>
Log &operator<<(Log &log, T &msg)
{
	static time_t timer;
	static tm ts;
	static wchar_t wasctmbuf[32];
	static errno_t en;

	if (log.isBOL)
	{
		time(&timer);
		localtime_s(&ts, &timer);
		en = _wasctime_s(wasctmbuf, &ts);

		if (en)
			throw en;
		
		wasctmbuf[wcslen(wasctmbuf) - 1] = L'\0';
		log.tmsg += L"[";
		log.tmsg += wasctmbuf;
		log.tmsg += L"] ";
	}

	log.tmsg += msg;

	return log;
}


Log &operator<<(Log &log, Log::_myendl &endl)
{
	log.isBOL = true;
	log.logQueue.push(log.tmsg);
	log.tmsg.clear();

	return log;
}


DWORD WINAPI Log::flushLogQueue(LPVOID lpParam)
{	
	while (NULL != lpParam && NULL != ((Log *) lpParam)->od && ((Log *)lpParam)->od->isInited)
	{
		if (((Log *)lpParam)->logQueue.size())
		{
			*(((Log *)lpParam)->od) << ((Log *)lpParam)->logQueue.front() << L"\n";
			((Log *)lpParam)->logQueue.pop();
		}
	}
	return 0;
}
