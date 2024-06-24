#include "server_backend.h"

#define IP_SERVER 127.0.0.1

int main(const int argc, const char* argv[]) {
    DIE(argc != 2, "Incorrect usage\n");

    // Disable buffering
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    uint16_t PORT_SERVER;
    int rc = sscanf(argv[1], "%hu", &PORT_SERVER);
    DIE(rc != 1, "Invalid server port");

    const int tcp_listen_socket = server::open_passive_socket(AF_INET, SOCK_STREAM, PORT_SERVER);
    const int udp_socket = server::open_passive_socket(PF_INET, SOCK_DGRAM, PORT_SERVER);

    /* Run server & start listening for connections */
    server::run_server(tcp_listen_socket, udp_socket);

    return 0;
}
