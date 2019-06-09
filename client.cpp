#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include "utils.h"
#include <sys/un.h>

const size_t BUFFER_SIZE = 65508;

const char* SOCKET_FILE = "/tmp/os-net-descriptor-passing";

int receive_descriptor(int descriptor) {
    struct msghdr msg{};
    struct iovec iov[1];
    char buffer[sizeof (int)], control[CMSG_SPACE(sizeof(int))];
    iov[0].iov_base = buffer;
    iov[0].iov_len = sizeof (buffer);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof (control);
    if (recvmsg(descriptor, &msg, 0) < 0) {
        print_error("recvmsg failed");
        return -1;
    }
    struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
    std::cout << "received " << (*(int*)CMSG_DATA(cm)) << " from " << descriptor << std::endl;
    return (*(int*)CMSG_DATA(cm));
}

int main() {
    descriptor_wrapper descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (descriptor == -1) {
        print_error("socket failed");
        return EXIT_FAILURE;
    }
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_FILE);
    descriptor_wrapper server{connect(descriptor, (const struct sockaddr *)&address, sizeof (address))};
    if (server < 0) {
        print_error("connect failed");
        return EXIT_FAILURE;
    }
    descriptor_wrapper write_pipe = receive_descriptor(descriptor);
    if (write_pipe < 0) {
        return EXIT_FAILURE;
    }
    descriptor_wrapper read_pipe = receive_descriptor(descriptor);
    if (read_pipe < 0) {
        return EXIT_FAILURE;
    }
    std::cout << "Type exit to exit \n";
    std::cout << "Type stop to stop server\n";
    char buffer[BUFFER_SIZE];
    while (true) {
        std::cout << "Type message to send: ";
        std::string line;
        getline(std::cin, line);
        if (std::cin.eof()) {
            break;
        }
        if (line.empty()) {
            continue;
        }
        write_all(line.c_str(), line.size() + 1, write_pipe);
        if (line == "exit") {
            break;
        }
        if (line == "stop") {
            break;
        }
        ssize_t len;
        std::string received;
        while ((len = read(read_pipe, &buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[len] = '\0';
            received += buffer;
            if (buffer[len - 1] == '\0') {
                break;
            }
        }
        std::cout << "response: " << received << std::endl;
    }

}
