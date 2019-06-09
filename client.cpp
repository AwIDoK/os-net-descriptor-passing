#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include "utils.h"

const size_t BUFFER_SIZE = 65508;

const char* SOCKET_FILE = "/tmp/os-net-descriptor-passing";

bool receive_descriptor(int descriptor) {
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
    return sendmsg(descriptor, &msg, 0) >= 0;
}

int main() {

}
