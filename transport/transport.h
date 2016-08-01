#ifndef TRANSPORT_TRANSPORT_H
#define TRANSPORT_TRANSPORT_H

#ifdef VERBS
#include "transport/verbs/socket.h"
#include "transport/verbs/acceptor.h"
#include "transport/verbs/connector.h"
#elif LIBFABRIC
#include "transport/libfabric/socket.h"
#include "transport/libfabric/acceptor.h"
#include "transport/libfabric/connector.h"
#else
static_assert(true, "Missing transport layer!");
#endif

#endif
