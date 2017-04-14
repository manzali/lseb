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

  std::cout << num_devices << " RDMA device(s) found:\n\n";

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

    std::cout << "The device '"
              << ibv_get_device_name(ctx->device)
              << "'  has "
              << device_attr.phys_port_cnt
              << " port(s)\n";

    if (ibv_close_device(ctx)) {
      std::cerr << "Error, failed to close the device '"
                << ibv_get_device_name(ctx->device)
                << "'\n";
      ibv_free_device_list(device_list);
      return EXIT_FAILURE;
    }
  }

  ibv_free_device_list(device_list);

  return EXIT_SUCCESS;
}
