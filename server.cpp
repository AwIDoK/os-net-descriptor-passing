#include <iostream>
#include <unistd.h>
#include "utils.h"
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>


const char* SOCKET_FILE = "/tmp/os-net-descriptor-passing";
const int BUFFER_SIZE = 65535;

bool send_descriptor(int client_descriptor, int fd_to_send) {
    std::cout << "Sending " << fd_to_send << " to " << client_descriptor << std::endl;
    struct msghdr msg{};
    struct iovec iov[1];
    char buffer[sizeof (int)], control[CMSG_SPACE(sizeof(int))];
    iov[0].iov_base = buffer;
    iov[0].iov_len = sizeof (buffer);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof (control);
    struct cmsghdr  *cm = CMSG_FIRSTHDR(&msg);
    cm->cmsg_len = CMSG_LEN(sizeof(int));
    cm->cmsg_level = SOL_SOCKET;
    cm->cmsg_type = SCM_RIGHTS;
    (*(int*)CMSG_DATA(cm)) = fd_to_send;
    return sendmsg(client_descriptor, &msg, 0) >= 0;
}

int main() {
    std::cout << "Starting server" << std::endl;
    descriptor_wrapper descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (descriptor == -1) {
        print_error("socket failed");
        return EXIT_FAILURE;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_FILE);
    unlink(SOCKET_FILE);
    if (bind(descriptor, (const struct sockaddr *)&address, sizeof (sockaddr_un)) < 0) {
        print_error("bind failed");
        return EXIT_FAILURE;
    }
    if (listen(descriptor, SOMAXCONN) < 0) {
        print_error("listen failed");
        return EXIT_FAILURE;
    }
    char buffer[BUFFER_SIZE];
    while (true) {
        descriptor_wrapper client_descriptor = accept(descriptor, nullptr, nullptr);
        std::cout << "client with descriptor " << client_descriptor << " connected\n";
        if (client_descriptor < 0) {
            print_error("accept failed");
            return EXIT_FAILURE;
        }
        int pipefd[2];
        int pipefd2[2];
        if (pipe(pipefd) < 0) {
            print_error("pipe failed");
            return EXIT_FAILURE;
        }
        if (pipe(pipefd2) < 0) {
            print_error("pipe failed");
            return EXIT_FAILURE;
        }
        descriptor_wrapper read_pipe(pipefd[0]);
        descriptor_wrapper client_write_pipe(pipefd[1]);
        descriptor_wrapper write_pipe(pipefd2[1]);
        descriptor_wrapper client_read_pipe(pipefd2[0]);
        if (!send_descriptor(client_descriptor, client_write_pipe)) {
            print_error("can't send descriptor");
            continue;
        }
        if (!send_descriptor(client_descriptor, client_read_pipe)) {
            print_error("can't send descriptor");
            continue;
        }
        client_write_pipe.~descriptor_wrapper();
        client_read_pipe.~descriptor_wrapper();
        ssize_t len;
        while ((len = read(read_pipe, &buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[len] = '\0';
            std::cout << "received from " << client_descriptor << ": " << buffer << std::endl;
            if (strcmp(buffer, "exit") == 0) {
                break;
            }
        }
        std::cout << "client with descriptor " << client_descriptor << " disconnected\n";
    }
}
