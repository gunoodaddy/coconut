#pragma once

namespace coconut {

typedef int ticket_t;

class RedisResponseImpl;

class COCONUT_API RedisResponse {
public:
	RedisResponse(void *reply, int ticket);
	RedisResponse(int err, const char *errmsg);
	~RedisResponse();

	typedef struct RedisReplyData {
		int type;
		long long integer;
		std::string str;
	}RedisReplyData;

private:
	void _load();

public:
	int resultErrorCode() const;
	const char* resultErrorMsg() const;
	const RedisReplyData * resultData() const;
	const RedisReplyData * resultDataOf(size_t index) const;
	ticket_t ticket() const;

private:
	RedisResponseImpl *impl_;
};

}
