#pragma once

namespace coconut {

class COCONUT_API Exception : public std::exception {
public:
	Exception(): code_(0) {}
	Exception(const char *message) : code_(0) {
		memcpy(message_, message, strlen(message));
	}
	Exception(int code, const char * message) : code_(code) {
		memcpy(message_, message, strlen(message));
	}

	virtual ~Exception() throw() {}

	virtual const char* what() const throw() {
		if (strlen(message_) <= 0) {
			return "Default Exception.";
		} else { 
			return message_;
		}
	}

	virtual int code() const throw() {
		return code_;
	}

protected:
	enum { MSG_SIZE = 128 };
	char message_[MSG_SIZE + 1];
	int code_;
};


class COCONUT_API ThreadException : public Exception { 
public:
	ThreadException(const char * message) : Exception(message) { }
}; 

class COCONUT_API SocketException : public Exception { 
public:
	SocketException(const char * message) : Exception(message) { }
}; 

class COCONUT_API RedisException : public Exception { 
public:
	RedisException(const char * message) : Exception(message) { }
	RedisException(int code, const char * message) : Exception(code, message) { }
}; 

class COCONUT_API IllegalStateException : public Exception { 
public:
	IllegalStateException(const char * message) : Exception(message) { }
}; 

class COCONUT_API IllegalArgumentException : public Exception { };

class COCONUT_API NoSuchElementException : public Exception { };

class COCONUT_API ProtocolException : public Exception { 
public:
	ProtocolException(const char * message) : Exception(message) { }
}; 

}
