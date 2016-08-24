//
// Created by Valentino on 17/08/16.
//

#include "domain.h"

#include <cassert>
#include <iostream>
#include <rdma/fi_errno.h>
#include <rdma/fi_domain.h>

namespace lseb {
Domain::Domain()
    : m_fabric(nullptr),
      m_domain(nullptr),
      m_info(nullptr, fi_freeinfo) {
  fi_info *hints = fi_allocinfo();
  // TODO: Add support for FI_DGRAM
  hints->caps = FI_MSG;
  hints->mode = FI_LOCAL_MR;
  hints->domain_attr->threading = FI_THREAD_COMPLETION;
  hints->domain_attr->data_progress = FI_PROGRESS_MANUAL;

  fi_info *info{nullptr};
  int rc = fi_getinfo(FI_VERSION(1,3), nullptr, nullptr, 0, hints, &info);

  // TODO: Improve error handling
  if (rc) {
    std::cerr << fi_strerror(-rc) << std::endl;
  }

  // TODO: Select provider based on configuration

  m_info.reset(info);

  fid_fabric *fabric{nullptr};
  rc = fi_fabric(m_info->fabric_attr, &fabric, nullptr);

  if (rc) {
    std::cerr << fi_strerror(-rc) << std::endl;
  }

  m_fabric.reset(fabric);

  fid_domain *domain{nullptr};

  rc = fi_domain(m_fabric.get(), m_info.get(), &domain, nullptr);

  if (rc) {
    std::cerr << fi_strerror(-rc) << std::endl;
  }

  m_domain.reset(domain);

  fi_freeinfo(hints);
}

Domain& Domain::get_instance() {
  static Domain instance;
  return instance;
}

const Domain::domain_ptr& Domain::get_domain() const {
  return m_domain;
}

fid_domain *Domain::get_raw_domain() const {
  return m_domain.get();
}

const Domain::fabric_ptr& Domain::get_fabric() const {
  return m_fabric;
}

fid_fabric *Domain::get_raw_fabric() const {
  return m_fabric.get();
}
fi_info *Domain::get_info() const {
  return m_info.get();
}

}