#include <iostream>

#include <cstdlib>

#include "generator/Generator.h"

static size_t const gigabyte = 1073741824;
static size_t const megabyte = 1048576;

int main() {
  size_t const metadata_size = gigabyte;
  char* metadata_ptr = new char[metadata_size];

  size_t const data_size = gigabyte;
  char* data_ptr = new char[data_size];

  Generator generator(metadata_ptr, metadata_size, data_ptr, data_size);

  int generated_events = generator.generateEvents();

  std::cout << "Generated " << generated_events << " events." << std::endl;

  delete[] metadata_ptr;
  delete[] data_ptr;
}

