#include "Coconut.h"
#include "IOService.h"
#include "RedisResponse.h"
#include <vector>
#include "Exception.h"

#if defined(WIN32)
#include <hiredis.h>
#else
#include <hiredis/hiredis.h>
#endif

namespace coconut {

class RedisResponseImpl {
public:
	RedisResponseImpl(RedisResponse *owner, struct redisReply *reply, ticket_t ticket) 
			: owner_(owner), reply_(reply), ticket_(ticket) { 
		_load();
	}

	~RedisResponseImpl() { }

	void _load() {
		if(reply_->type != REDIS_REPLY_ARRAY) {
			struct RedisResponse::RedisReplyData result;
			result.type = reply_->type;
			result.integer = reply_->integer;
			if(reply_->len > 0)
				result.str.assign(reply_->str, reply_->len);
			results_.push_back(result);
		} else {
			for(size_t i = 0; i < reply_->elements; i++) {
				struct RedisResponse::RedisReplyData result;
				redisReply *element = reply_->element[i];
				result.type = element->type;
				result.integer = element->integer;
				if(element->len > 0)
					result.str.assign(element->str, element->len);
				results_.push_back(result);
			}
		}
	}

	const RedisResponse::RedisReplyData * result() const { 
		assert(results_.size() > 0 && "RedisResponse result array size <= 0");
		return &results_[0];
	}

	const RedisResponse::RedisReplyData * resultOf(size_t index) const {
		if(index >= results_.size()) 
			throw IllegalArgumentException();
		return &results_[index];
	}

	ticket_t ticket() const {
		return ticket_;
	}

private:
	RedisResponse *owner_;
	std::vector<RedisResponse::RedisReplyData> results_;
	redisReply *reply_;
	int ticket_;
};

//-------------------------------------------------------------------------------------------------------

RedisResponse::RedisResponse(void *reply, int ticket) {
	impl_ = new RedisResponseImpl(this, (struct redisReply *)reply, ticket);
}

RedisResponse::~RedisResponse() {
	delete impl_;
}

const RedisResponse::RedisReplyData* RedisResponse::result() const {
	return impl_->result();
}

const RedisResponse::RedisReplyData* RedisResponse::resultOf(size_t index) const {
	return impl_->resultOf(index);
}

ticket_t RedisResponse::ticket() const {
	return impl_->ticket();
}

}
