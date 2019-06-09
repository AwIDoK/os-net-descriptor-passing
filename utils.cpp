#include "utils.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <numeric>

void print_error(std::string const& message) {
    std::cerr << message << std::strerror(errno) << std::endl;
}

void close_socket(int descriptor) {
    if (close(descriptor) == -1) {
        print_error("Can't close socket");
    }
}
