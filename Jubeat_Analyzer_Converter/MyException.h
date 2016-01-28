#pragma once

#include <exception>

class MyException : public std::exception
{
		const char* _reason;
		bool _iscopy;
	public:
		MyException(const char* reason, bool copy = false);
		MyException(const MyException& exc);
		~MyException();
		const char* what() const;
};
