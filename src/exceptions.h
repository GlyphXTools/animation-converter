#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "types.h"
#include "utils.h"
#include "resource.h"
#include <string>
#include <stdexcept>

class wexception : public std::exception
{
	std::wstring message;
public:
	const wchar_t* what() { return message.c_str(); }
	wexception(const wchar_t* _message) : message(_message) {}
	wexception(const std::wstring& _message) : message(_message) {}
};

class wruntime_error : public wexception
{
public:
	wruntime_error(const std::wstring& message) : wexception(message) {}
};

class IOException : public wruntime_error
{
public:
	IOException(const std::wstring& message) : wruntime_error(message) {}
};

class ParseException : public wruntime_error
{
public:
	ParseException(const std::wstring& message) : wruntime_error(message) {}
};

class FileNotFoundException : public IOException
{
public:
	FileNotFoundException(const std::wstring filename) : IOException(L"Unable to find file:\n" + filename) {}
};

class ReadException : public IOException
{
public:
	ReadException() : IOException(LoadString(IDS_ERROR_FILE_READ)) {}
};

class WriteException : public IOException
{
public:
	WriteException() : IOException(LoadString(IDS_ERROR_FILE_WRITE)) {}
};

class BadFileException : public IOException
{
public:
	BadFileException() : IOException(LoadString(IDS_ERROR_FILE_CORRUPT)) {}
};

#endif