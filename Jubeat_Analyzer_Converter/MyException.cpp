#include "MyException.h"
#include <cstring>

MyException::MyException(const char* reason, bool copy)
	: _reason(reason), _iscopy(copy)
{
	if (copy)
	{
		int lenReason = std::strlen(reason);
		char* r = new char[lenReason + 1];
		strcpy(r, reason);
		_reason = r;
	}
}

MyException::MyException(const MyException& exc)
{
	_iscopy = exc._iscopy;
	if (_iscopy) {
		int len = std::strlen(exc._reason);
		char* r = new char[len + 1];
		strcpy(r, exc._reason);
		_reason = r;
	} else
		_reason = exc._reason;
}

MyException::~MyException()
{
	if(_iscopy)
		delete[] _reason;
}

const char* MyException::what() const
{
	return _reason;
}
