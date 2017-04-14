#include <iostream>
#include <cstdlib>
#include <infiniband/verbs.h>

int main() {

  int num_devices;
  ibv_device** device_list = ibv_get_device_list(&num_devices);

  if (!device_list) {
    std::cerr << "Error, ibv_get_device_list() failed\n";
    return EXIT_FAILURE;
  }

  std::cout << num_devices << " RDMA device(s) found.\n\n";

  for (int i = 0; i < num_devices; ++i) {

    ibv_device_attr device_attr;

    ibv_context* ctx = ibv_open_device(device_list[i]);
    if (!ctx) {
      std::cerr << "Error, failed to open the device '"
                << ibv_get_device_name(device_list[i])
                << "'\n";
      ibv_free_device_list(device_list);
      return EXIT_FAILURE;
    }

    if (ibv_query_device(ctx, &device_attr)) {
      std::cerr << "Error, failed to query the device '"
                << ibv_get_device_name(device_list[i])
                << "' attributes\n";
      ibv_close_device(ctx);
      ibv_free_device_list(device_list);
      return EXIT_FAILURE;
    }

    std::cout << "\ndevice " << ibv_get_device_name(ctx->device) << ":\n";

    std::cout << "    fw_ver: " << device_attr.fw_ver << std::endl;
    std::cout << "    node_guid: " << device_attr.node_guid << std::endl;
    std::cout << "    sys_image_guid: " << device_attr.sys_image_guid << std::endl;
    std::cout << "    max_mr_size: " << device_attr.max_mr_size << std::endl;
    std::cout << "    page_size_cap: " << device_attr.page_size_cap << std::endl;
    std::cout << "    vendor_id: " << device_attr.vendor_id << std::endl;
    std::cout << "    vendor_part_id: " << device_attr.vendor_part_id << std::endl;
    std::cout << "    hw_ver: " << device_attr.hw_ver << std::endl;
    std::cout << "    max_qp: " << device_attr.max_qp << std::endl;
    std::cout << "    max_qp_wr: " << device_attr.max_qp_wr << std::endl;
    std::cout << "    device_cap_flags: "
              << device_attr.device_cap_flags
              << std::endl;
    std::cout << "    max_sge_rd: " << device_attr.max_sge_rd << std::endl;
    std::cout << "    max_cq: " << device_attr.max_cq << std::endl;
    std::cout << "    max_cqe: " << device_attr.max_cqe << std::endl;
    std::cout << "    max_mr: " << device_attr.max_mr << std::endl;
    std::cout << "    max_pd: " << device_attr.max_pd << std::endl;
    std::cout << "    max_qp_rd_atom: " << device_attr.max_qp_rd_atom << std::endl;
    std::cout << "    max_ee_rd_atom: " << device_attr.max_ee_rd_atom << std::endl;
    std::cout << "    max_res_rd_atom: "
              << device_attr.max_res_rd_atom
              << std::endl;
    std::cout << "    max_qp_init_rd_atom: "
              << device_attr.max_qp_init_rd_atom
              << std::endl;
    std::cout << "    max_ee_init_rd_atom: "
              << device_attr.max_ee_init_rd_atom
              << std::endl;
    std::cout << "    max_ee: " << device_attr.max_ee << std::endl;
    std::cout << "    max_rdd: " << device_attr.max_rdd << std::endl;
    std::cout << "    max_mw: " << device_attr.max_mw << std::endl;
    std::cout << "    max_raw_ipv6_qp: "
              << device_attr.max_raw_ipv6_qp
              << std::endl;
    std::cout << "    max_raw_ethy_qp: "
              << device_attr.max_raw_ethy_qp
              << std::endl;
    std::cout << "    max_mcast_grp: " << device_attr.max_mcast_grp << std::endl;
    std::cout << "    max_mcast_qp_attach: "
              << device_attr.max_mcast_qp_attach
              << std::endl;
    std::cout << "    max_total_mcast_qp_attach: "
              << device_attr.max_total_mcast_qp_attach
              << std::endl;
    std::cout << "    max_ah: " << device_attr.max_ah << std::endl;
    std::cout << "    max_fmr: " << device_attr.max_fmr << std::endl;
    std::cout << "    max_map_per_fmr: "
              << device_attr.max_map_per_fmr
              << std::endl;
    std::cout << "    max_srq: " << device_attr.max_srq << std::endl;
    std::cout << "    max_srq_wr: " << device_attr.max_srq_wr << std::endl;
    std::cout << "    max_srq_sge: " << device_attr.max_srq_sge << std::endl;
    std::cout << "    max_pkeys: " << device_attr.max_pkeys << std::endl;
    std::cout << "    local_ca_ack_delay: "
              << static_cast<uint32_t>(device_attr.local_ca_ack_delay)
              << std::endl;
    std::cout << "    phys_port_cnt: "
              << static_cast<uint32_t>(device_attr.phys_port_cnt)
              << std::endl;

    if (ibv_close_device(ctx)) {
      std::cerr << "Error, failed to close the device '"
                << ibv_get_device_name(ctx->device)
                << "'\n";
      ibv_free_device_list(device_list);
      return EXIT_FAILURE;
    }

    std::cout << std::endl;
  }

  ibv_free_device_list(device_list);

  return EXIT_SUCCESS;
}
