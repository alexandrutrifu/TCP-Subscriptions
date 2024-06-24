#include "subscriber_backend.h"

int main(const int argc, const char *argv[]) {
    DIE(argc != 4, "Incorrect usage\n");

    // Disable buffering
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    /* Parse executable arguments */
    char client_id[MAX_ID_LEN];
    char ip_server[MAX_IP_LEN];
    uint16_t PORT_SERVER;

    int rc = sscanf(argv[1], "%s", client_id);
    DIE(rc != 1, "Invalid client ID");

    rc = sscanf(argv[2], "%s", ip_server);
    DIE(rc != 1, "Invalid server IP");

    sscanf(argv[3], "%hu", &PORT_SERVER);
    DIE(rc != 1, "Invalid server PORT");

    subscriber::login_subscriber(client_id, inet_addr(ip_server), PORT_SERVER);

    return 0;
}