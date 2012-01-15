#pragma once

namespace coconut {

class COCONUT_API Exception : public std::exception {
public:
	Exception(): message_(), code_(0) {}
	Exception(const std::string& message) : message_(message), code_(0) {}
	Exception(int code, const std::string& message) : message_(message), code_(code) {}

	virtual ~Exception() throw() {}

	virtual const char* what() const throw() {
		if (message_.empty()) {
			return "Default Exception.";
		} else { 
			return message_.c_str();
		}
	}

	virtual int code() const throw() {
		return code_;
	}

protected:
	std::string message_;
	int code_;
};


class COCONUT_API ThreadException : public Exception { 
public:
	ThreadException(const std::string& message) : Exception(message) { }
}; 

class COCONUT_API SocketException : public Exception { 
public:
	SocketException(const std::string& message) : Exception(message) { }
}; 

class COCONUT_API RedisException : public Exception { 
public:
	RedisException(const std::string& message) : Exception(message) { }
	RedisException(int code, const std::string& message) : Exception(code, message) { }
}; 

class COCONUT_API IllegalStateException : public Exception { 
public:
	IllegalStateException(const std::string& message) : Exception(message) { }
}; 

class COCONUT_API IllegalArgumentException : public Exception { };

class COCONUT_API NoSuchElementException : public Exception { };

class COCONUT_API ProtocolException : public Exception { 
public:
	ProtocolException(const std::string& message) : Exception(message) { }
}; 

}
