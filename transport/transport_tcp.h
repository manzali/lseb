#ifndef TRANSPORT_TRANSPORT_TCP_H
#define TRANSPORT_TRANSPORT_TCP_H

#include <vector>
#include <iostream>
#include <sys/poll.h>
#include <sys/uio.h>

namespace lseb {

int lseb_connect(std::string const& hostname, long port);
int lseb_listen(std::string const& hostname, long port);
int lseb_accept(int sockfd);
int lseb_poll(std::vector<pollfd>& poll_fds, int timeout_ms);
ssize_t lseb_write(int sockfd, std::vector<iovec> const& iov);
ssize_t lseb_read(int sockfd, void* buffer, size_t nbytes);

}

#endif
