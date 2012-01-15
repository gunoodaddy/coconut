#pragma once

namespace coconut {

typedef int ticket_t;

class RedisResponseImpl;

class COCONUT_API RedisResponse {
public:
	RedisResponse(void *reply, int ticket);
	~RedisResponse();

	typedef struct RedisReplyData {
		int type;
		long long integer;
		std::string str;
	}RedisReplyData;

private:
	void _load();

public:
	const RedisReplyData * result() const;
	const RedisReplyData * resultOf(size_t index) const;
	ticket_t ticket() const;

private:
	RedisResponseImpl *impl_;
};

}
