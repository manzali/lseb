//
// Created by Valentino on 17/08/16.
//

#ifndef LSEB_DOMAIN_HPP
#define LSEB_DOMAIN_HPP

#include <memory>
#include "rdma/fabric.h"
#include "shared.h"

namespace lseb {

/* Singleton class */

/* NOTE: Already opened fabric and domain can be retrieved with fi_getinfo */

class Domain {
public:

  typedef std::unique_ptr<fid_fabric, fid_deleter<fid_fabric>> fabric_ptr;
  typedef std::unique_ptr<fid_domain, fid_deleter<fid_domain>> domain_ptr;
  typedef std::unique_ptr<fi_info, decltype(&fi_freeinfo)> info_ptr;

  static Domain& get_instance();
  Domain(Domain const&) = delete;             // Copy construct
  Domain(Domain&&) = delete;                  // Move construct
  Domain& operator=(Domain const&) = delete;  // Copy assign
  Domain& operator=(Domain&&) = delete;      // Move assign

  const domain_ptr& get_domain() const;
  fid_domain *get_raw_domain() const;
  const fabric_ptr& get_fabric() const;
  fid_fabric *get_raw_fabric() const;
  fi_info *get_hints() const;
private:
  Domain();
  ~Domain() = default;

  fabric_ptr m_fabric;
  domain_ptr m_domain;
  info_ptr m_hints;

};

}
#endif //LSEB_DOMAIN_HPP
