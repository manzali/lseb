#ifndef TRANSPORT_TRANSPORT_H
#define TRANSPORT_TRANSPORT_H

#ifdef TCP
#include "transport/transport_tcp.h"
#elif RSOCKETS
#include "transport/transport_rsockets.h"
#endif

#endif
