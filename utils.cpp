#include "utils.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <numeric>

void print_error(std::string const& message) {
    std::cerr << message  << ": " << std::strerror(errno) << std::endl;
}

void close_socket(int descriptor) {
    if (close(descriptor) == -1) {
        print_error("Can't close socket");
    }
}

void write_all(const char* pointer, size_t len, int descriptor) {
    size_t result;
    while (len && (result = write(descriptor, pointer, len))) {
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result < 0) {
            exit(EXIT_FAILURE);
        }
        len -= result;
        pointer += result;
    }
}
