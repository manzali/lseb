#include <iostream>

#include <rdma/fabric.h>

int main() {

  fi_info *info { nullptr };
  fi_getinfo(fi_version(), nullptr, nullptr, 0, nullptr, &info);

  for (fi_info *it = info; it != nullptr; it = it->next) {

    std::cout << "\navailable service:\n"
              << fi_tostr(it, FI_TYPE_INFO)
              << std::endl;

/*
    std::cout << "    control_progress: "
              << fi_tostr(&it->domain_attr->control_progress, FI_TYPE_PROGRESS)
              << std::endl;

    std::cout << "  fabric_attr:\n";
    std::cout << "    name: " << it->fabric_attr->name << std::endl;
    std::cout << "    prov_name: " << it->fabric_attr->prov_name << std::endl;
    std::cout << "    prov_version: "
              << FI_MAJOR(it->fabric_attr->prov_version)
              << "."
              << FI_MINOR(it->fabric_attr->prov_version)
              << std::endl;

    std::cout << "  domain_attr:\n";
    std::cout << "    name: " << it->domain_attr->name << std::endl;
    std::cout << "    threading: "
              << fi_tostr(&it->domain_attr->threading, FI_TYPE_THREADING)
              << std::endl;
    std::cout << "    control_progress: "
              << fi_tostr(&it->domain_attr->control_progress, FI_TYPE_PROGRESS)
              << std::endl;
    std::cout << "    data_progress: "
              << fi_tostr(&it->domain_attr->data_progress, FI_TYPE_PROGRESS)
              << std::endl;
    std::cout << "    av_type: "
              << fi_tostr(&it->domain_attr->av_type, FI_TYPE_AV_TYPE)
              << std::endl;

    std::cout << "    mr_key_size: "
              << it->domain_attr->mr_key_size
              << std::endl;
    std::cout << "    cq_data_size: "
              << it->domain_attr->cq_data_size
              << std::endl;
    std::cout << "    cq_cnt: " << it->domain_attr->cq_cnt << std::endl;
    std::cout << "    ep_cnt: " << it->domain_attr->ep_cnt << std::endl;
    std::cout << "    tx_ctx_cnt: " << it->domain_attr->tx_ctx_cnt << std::endl;
    std::cout << "    rx_ctx_cnt: " << it->domain_attr->rx_ctx_cnt << std::endl;
    std::cout << "    max_ep_tx_ctx: "
              << it->domain_attr->max_ep_tx_ctx
              << std::endl;
    std::cout << "    max_ep_rx_ctx: "
              << it->domain_attr->max_ep_rx_ctx
              << std::endl;
    std::cout << "    max_ep_stx_ctx: "
              << it->domain_attr->max_ep_stx_ctx
              << std::endl;
    std::cout << "    max_ep_srx_ctx: "
              << it->domain_attr->max_ep_srx_ctx
              << std::endl;

    std::cout << "  ep_attr:\n";
    std::cout << "    type: "
              << fi_tostr(&it->ep_attr->type, FI_TYPE_EP_TYPE)
              << std::endl;
    std::cout << "    protocol: "
              << fi_tostr(&it->ep_attr->protocol, FI_TYPE_PROTOCOL)
              << std::endl;
    std::cout << "    protocol: "
              << fi_tostr(&it->ep_attr->protocol, FI_TYPE_PROTOCOL)
              << std::endl;
*/
  }
  fi_freeinfo(info);

  return 0;
}
