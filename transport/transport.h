#ifndef TRANSPORT_TRANSPORT_H
#define TRANSPORT_TRANSPORT_H

#ifdef TCP

#else

#include <iostream>

#endif

namespace lseb {

int lseb_connect(std::string hostname, long port);
int lseb_listen(std::string hostname, long port);
int lseb_accept(int conn_id);

}

#endif
