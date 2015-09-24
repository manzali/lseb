#ifndef TRANSPORT_TRANSPORT_H
#define TRANSPORT_TRANSPORT_H

#ifdef TCP

#elif VERBS
#include "transport/verbs/socket_verbs.h"
#include "transport/verbs/acceptor_verbs.h"
#include "transport/verbs/connector_verbs.h"
#else
static_assert(true, "Missing transport layer!");
#endif

#endif
