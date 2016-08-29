//
// Created by Valentino on 05/08/16.
//

#ifndef TRANSPORT_LIBFABRIC_SHARED_H
#define TRANSPORT_LIBFABRIC_SHARED_H

#include <type_traits>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>

#include "traits.h"

namespace lseb {

template<typename T, typename std::enable_if<is_fabric_id<T>::value,
                                             T>::type * = nullptr>
struct fid_deleter {
  void operator()(T *fid) {
    // TODO: check return value of fi_close
    fi_close(&fid->fid);
  }
};

template<>
struct fid_deleter<fi_info, nullptr> {
  void operator()(fi_info *fid) {
    do {
      fi_info* next = fid->next;
      fi_freeinfo(fid);
      fid = next;
    } while(fid);
  }
};

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
};

}

#endif //TRANSPORT_LIBFABRIC_SHARED_H