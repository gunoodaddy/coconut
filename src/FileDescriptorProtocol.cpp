#include "Coconut.h"
#include "FileDescriptorProtocol.h"
#include "BaseSocket.h"
#if ! defined(WIN32)
#include <arpa/inet.h>
#endif

namespace coconut { namespace protocol {

bool FileDescriptorProtocol::processWrite(boost::shared_ptr<BaseVirtualTransport> transport) {
#if defined(WIN32)	
	assert(false && "Windows not support Unix Domain Socket");
	return false;
#else
	if(strcmp(transport->className(), "BaseSocket") != 0) {
		throw ProtocolException("Invalid class name of transport");
	}
	boost::shared_ptr<BaseSocket> socket = boost::static_pointer_cast<BaseSocket>(transport);

	unsigned char data[2] = { 0xab, 0xcd };
	struct msghdr   msg;
	struct iovec    iov[1];

	union {
		struct cmsghdr    cm;
		char              control[CMSG_SPACE(sizeof(int))];
	} control_un;
	struct cmsghdr  *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);

	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*((int *) CMSG_DATA(cmptr)) = fd_;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = data;
	iov[0].iov_len = 1;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	::sendmsg(socket->socketFD(), &msg, 0);

	LOG_DEBUG("write descritor : %d -> %d", socket->socketFD(), fd_);
	return true;
#endif
}

bool FileDescriptorProtocol::processRead(boost::shared_ptr<BaseVirtualTransport> transport) {

#if defined(WIN32)	
	assert(false && "Windows not support Unix Domain Socket");
	return false;
#else
	if(strcmp(transport->className(), "BaseSocket") != 0) {
		throw ProtocolException("Invalid class name of transport");
	}

	boost::shared_ptr<BaseSocket> socket = boost::static_pointer_cast<BaseSocket>(transport);

	char tempBuf;	
	struct msghdr msg;
	struct iovec iov[1];
	ssize_t n;
	union {
		struct cmsghdr    cm;
		char              control[CMSG_SPACE(sizeof(int))];
	} control_un;
	struct cmsghdr  *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = &tempBuf;
	iov[0].iov_len = 1;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	if ( (n = recvmsg(socket->socketFD(), &msg, 0)) <= 0) {
		socket->checkResponseSocket(n);
		return false;
	}

	if ( (cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
			cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
		fd_ = *((int *) CMSG_DATA(cmptr));
		readComplete_ = true;
		return true;
	}

	return false;
#endif
}

}}

