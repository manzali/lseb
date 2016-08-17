//
// Created by Valentino on 17/08/16.
//

#ifndef LSEB_DOMAIN_HPP
#define LSEB_DOMAIN_HPP

namespace lsev {

class Domain {
public:
  Domain& get_instance();
  Domain(Domain const&) = delete;             // Copy construct
  Domain(Domain&&) = delete;                  // Move construct
  Domain& operator=(Domain const&) = delete;  // Copy assign
  Domain& operator=(Domain &&) = delete;      // Move assign
private:
  Domain();
  ~Domain();

};

}

#endif //LSEB_DOMAIN_HPP
