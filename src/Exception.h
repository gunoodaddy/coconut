/*
* Copyright (c) 2012, Eunhyuk Kim(gunoodaddy) 
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   * Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of Redis nor the names of its contributors may be used
*     to endorse or promote products derived from this software without
*     specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

namespace coconut {

class COCONUT_API Exception : public std::exception {
public:
	Exception(): code_(0) {}
	Exception(const char *message) : code_(0) {
		setMessage(message);
	}
	Exception(int code, const char * message) : code_(code) {
		setMessage(message);
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

private:
	void setMessage(const char* msg) {
		size_t len = strlen(msg);
		if(len > sizeof(message_) - 1)
			len = sizeof(message_) - 1;
		memset(message_, 0, sizeof(message_));
		memcpy(message_, msg, len);    
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
