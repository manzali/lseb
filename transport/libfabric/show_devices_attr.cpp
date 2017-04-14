#include <iostream>

#include <rdma/fabric.h>

int main() {

  fi_info *info { nullptr };
  fi_getinfo(fi_version(), nullptr, nullptr, 0, nullptr, &info);

  for (fi_info *it = info; it != nullptr; it = it->next) {

    std::cout << "available service:\n";

    // print provider name
    std::cout << "\tprovider name: " << it->fabric_attr->prov_name << std::endl;

    // print fabric identifier
    std::cout << "\tfabric identifier: " << it->fabric_attr->name << std::endl;

    // print version
    std::cout << "\tversion: "
              << FI_MAJOR(it->fabric_attr->prov_version)
              << "."
              << FI_MINOR(it->fabric_attr->prov_version)
              << std::endl;

    // print endpoint type
    std::cout << "\tendpoint type: "
              << fi_tostr(&it->ep_attr->type, FI_TYPE_EP_TYPE)
              << std::endl;

    // print protocol
    std::cout << "\tprotocol: "
              << fi_tostr(&it->ep_attr->protocol, FI_TYPE_PROTOCOL)
              << std::endl;

  }
  fi_freeinfo(info);

  return 0;
}
