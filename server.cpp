#include <iostream>
#include <unistd.h>
#include "utils.h"
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>


const char* SOCKET_FILE = "/tmp/os-net-descriptor-passing";

bool send_descriptor(int socket_descriptor, int descriptor) {
    struct msghdr msg{};
    struct iovec iov{};
    char buffer[sizeof (int)], control[CMSG_SPACE(sizeof(int))];
    iov.iov_base = buffer;
    iov.iov_len = 1;
    msg.msg_iov = &iov;
    msg.msg_iovlen = sizeof (iov);
    msg.msg_control = control;
    msg.msg_controllen = sizeof (control);
    struct cmsghdr  *cm;
    cm = CMSG_FIRSTHDR(&msg);
    cm->cmsg_len = CMSG_LEN(sizeof(int));
    cm->cmsg_level = SOL_SOCKET;
    cm->cmsg_type = SCM_RIGHTS;
    mempcpy(CMSG_DATA(cm), reinterpret_cast<char*>(descriptor), sizeof (int));
    return sendmsg(socket_descriptor, &msg, 0) >= 0;
}

int main() {
    std::cout << "Starting server" << std::endl;
    descriptor_wrapper descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (descriptor == -1) {
        print_error("socket failed: ");
        return EXIT_FAILURE;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_FILE);
    if (bind(descriptor, (const struct sockaddr *)&address, sizeof (sockaddr_un)) < 0) {
        print_error("bind failed: ");
        return EXIT_FAILURE;
    }

    if (listen(descriptor, SOMAXCONN) < 0) {
        print_error("listen failed: ");
        return EXIT_FAILURE;
    }
    while (true) {
        int client_descriptor = accept(descriptor, nullptr, nullptr);
        if (client_descriptor < 0) {
            print_error("accept failed: ");
            return EXIT_FAILURE;
        }
        send_descriptor(descriptor, client_descriptor);
    }
}
