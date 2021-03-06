#include "domain.h"

#include <string>
#include <cassert>
#include <iostream>

#include <string.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_domain.h>

#include "common/exception.h"

namespace lseb {
Domain::Domain()
    : m_fabric(nullptr),
      m_domain(nullptr),
      m_hints(nullptr) {

  // TODO: Add support for FI_DGRAM
  m_hints.reset(fi_allocinfo());
  m_hints->caps = FI_MSG;
  m_hints->mode = FI_LOCAL_MR;
  m_hints->ep_attr->type = FI_EP_MSG;
  m_hints->domain_attr->threading = FI_THREAD_COMPLETION;
  m_hints->domain_attr->data_progress = FI_PROGRESS_MANUAL;
#ifdef FI_VERBS
  m_hints->fabric_attr->prov_name = strdup("verbs");
#else // FI_TCP
  m_hints->fabric_attr->prov_name = strdup("sockets");
#endif

  fi_info* info_p;
  int rc = fi_getinfo(
      FI_VERSION(1, 3),
      nullptr,
      nullptr,
      0,
      m_hints.get(),
      &info_p);

  fabric_ptr<fi_info> info { info_p };
  // TODO: Improve error handling
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on Domain fi_info: " + std::string(fi_strerror(-rc)));
  }

  fid_fabric* fabric;
  rc = fi_fabric(info->fabric_attr, &fabric, nullptr);
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on Domain fi_fabric: " + std::string(fi_strerror(-rc)));
  }
  m_fabric.reset(fabric);

  fid_domain* domain;
  rc = fi_domain(m_fabric.get(), info.get(), &domain, nullptr);
  if (rc) {
    throw lseb::exception::connection::generic_error(
        "Error on Domain fi_domain: " + std::string(fi_strerror(-rc)));
  }
  m_domain.reset(domain);
}

Domain& Domain::get_instance() {
  static Domain instance;
  return instance;
}

const Domain::domain_ptr& Domain::get_domain() const {
  return m_domain;
}

fid_domain* Domain::get_raw_domain() const {
  return m_domain.get();
}

const Domain::fab_ptr& Domain::get_fabric() const {
  return m_fabric;
}

fid_fabric* Domain::get_raw_fabric() const {
  return m_fabric.get();
}
fi_info* Domain::get_hints() const {
  return m_hints.get();
}

}
