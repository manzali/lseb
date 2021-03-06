#ifndef TRANSPORT_LIBFABRIC_SHARED_H
#define TRANSPORT_LIBFABRIC_SHARED_H

#include <type_traits>
#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>

#include "traits.h"

namespace lseb {

template<typename T,
    typename std::enable_if<is_fabric_id<T>::value, T>::type * = nullptr>
struct fid_deleter {
  void operator()(T *fid) {
    // TODO: check return value of fi_close
    fi_close(&fid->fid);
  }
};

template<>
struct fid_deleter<fi_info, nullptr> {
  void operator()(fi_info *fid) {
    fi_freeinfo(fid);
  }
};

template<typename T, typename ... Args>
std::unique_ptr<T> make_unique(Args&& ... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
;

// Unique pointer to fabric resource
template<typename T>
using fabric_ptr = std::unique_ptr<T, fid_deleter<T>>;

// Helper functions for endpoint creation and setup
void read_event(
    fabric_ptr<fid_eq> const& eq,
    fi_eq_cm_entry *entry,
    uint32_t event);
void bind_completion_queues(
    fabric_ptr<fid_ep> const& ep,
    fabric_ptr<fid_cq>& rx,
    fabric_ptr<fid_cq>& tx,
    uint32_t size);
void bind_event_queue(fabric_ptr<fid_ep> const& ep, fabric_ptr<fid_eq>& eq);

}

#endif //TRANSPORT_LIBFABRIC_SHARED_H
