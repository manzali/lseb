//
// Created by Valentino on 05/08/16.
//

#ifndef TRANSPORT_LIBFABRIC_SHARED_H
#define TRANSPORT_LIBFABRIC_SHARED_H

#include <type_traits>
#include <rdma/fabric.h>

#include "traits.h"

namespace lseb {

template<typename T, typename std::enable_if<is_fabric_id<T>::value,
                                             T>::type * = nullptr>
struct delete_fid {
  void operator()(T *fid) {
    // TODO: check return value of fi_close
    fi_close(&fid->fid);
  }
};

}

#endif //TRANSPORT_LIBFABRIC_SHARED_H
