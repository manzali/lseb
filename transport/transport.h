#ifndef TRANSPORT_TRANSPORT_H
#define TRANSPORT_TRANSPORT_H

#ifdef TCP
#include "transport/tcp/socket_tcp.h"
#include "transport/tcp/acceptor_tcp.h"
#include "transport/tcp/connector_tcp.h"
#elif VERBS
#include "transport/verbs/socket_verbs.h"
#include "transport/verbs/acceptor_verbs.h"
#include "transport/verbs/connector_verbs.h"
#elif LIBFABRIC
#include "transport/verbs/socket_libfabric.h"
#include "transport/verbs/acceptor_libfabric.h"
#include "transport/verbs/connector_libfabric.h"
#else
static_assert(true, "Missing transport layer!");
#endif

#endif
