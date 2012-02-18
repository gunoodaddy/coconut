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

#include "CoconutLib.h"
#ifdef HAVE_LIBHIREDIS
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
			: owner_(owner), reply_(reply), ticket_(ticket), err_(0), errmsg_() { 
		_load();
	}

	RedisResponseImpl(RedisResponse *owner, int err, const char *errmsg, ticket_t ticket)
			: owner_(owner), reply_(NULL), ticket_(ticket), err_(err), errmsg_(errmsg) { 
		_load();
	}

	~RedisResponseImpl() { }

	void _load() {
		if(!reply_) {
			return;	// error response case
		}

		if(reply_->type != REDIS_REPLY_ARRAY) {
			struct RedisResponse::RedisReplyData result;
			result.type = reply_->type;
			result.bigIntValue = reply_->integer;
			result.intValue = (int)reply_->integer;
			if(reply_->len > 0)
				result.strValue.assign(reply_->str, reply_->len);
			results_.push_back(result);
		} else {
			for(size_t i = 0; i < reply_->elements; i++) {
				struct RedisResponse::RedisReplyData result;
				redisReply *element = reply_->element[i];
				result.type = element->type;
				result.bigIntValue = reply_->integer;
				result.intValue = (int)element->integer;
				if(element->len > 0)
					result.strValue.assign(element->str, element->len);
				results_.push_back(result);
			}
		}
	}

	int resultErrorCode() const {
		return err_;
	}

	size_t resultDataCount() const {
		return results_.size();
	}

	const char* resultErrorMsg() const {
		return errmsg_.c_str();
	}

	const RedisResponse::RedisReplyData * resultData() const { 
		if(results_.size() > 0)
			return &results_[0];
		return NULL;
	}

	const RedisResponse::RedisReplyData * resultDataOf(size_t index) const {
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
	int err_;
	std::string errmsg_;
};

//-------------------------------------------------------------------------------------------------------

RedisResponse::RedisResponse(void *reply, ticket_t ticket) {
	impl_ = new RedisResponseImpl(this, (struct redisReply *)reply, ticket);
}

RedisResponse::RedisResponse(int err, const char *errmsg, ticket_t ticket) {
	impl_ = new RedisResponseImpl(this, err, errmsg, ticket);
}

RedisResponse::~RedisResponse() {
	delete impl_;
}

int RedisResponse::resultErrorCode() const {
	return impl_->resultErrorCode();
}

const char* RedisResponse::resultErrorMsg() const {
	return impl_->resultErrorMsg();
}

const RedisResponse::RedisReplyData* RedisResponse::resultData() const {
	return impl_->resultData();
}

const RedisResponse::RedisReplyData* RedisResponse::resultDataOf(size_t index) const {
	return impl_->resultDataOf(index);
}

size_t RedisResponse::resultDataCount() const {
	return impl_->resultDataCount();
}

ticket_t RedisResponse::ticket() const {
	return impl_->ticket();
}

}
#endif
