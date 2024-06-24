#ifndef SUBSCRIBER_BACKEND_H
#define SUBSCRIBER_BACKEND_H

#include "subscription_protocol.h"

using namespace subscription_protocol;

namespace subscriber {
    /**
     * Opens new socket and connects the subscriber to the server.
     *
     * @param ip_server
     * @param PORT
     * @return
     */
    int connect_to_server(uint32_t ip_server, const uint16_t PORT, char *client_ID) {
        /* Create new TCP socket */
        const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        DIE(socket_fd < 0, "Subscriber socket error");

        /* Make socket address reusable */
		const int enable = 1;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			perror("setsockopt(SO_REUSEADDR) failed");

        if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
			perror("setsockopt(TCP_NODELAY) failed");

        /* Fill in server details */
        struct sockaddr_in server_addr = {};

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = ip_server;

        /* Attempt server connection */
        int rc = connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        DIE (rc < 0, "Connection to server failed");

        /* Send confirmation ID to server */
        subscription_packet packet{};
        sprintf(packet.message, "%s", client_ID);
        packet.length = strlen(client_ID);
        connection::send_full_message(socket_fd, (void *)&packet, sizeof(packet));

        /* Await server confirmation */
        connection::receive_full_message(socket_fd, (void *)&packet, sizeof(packet));

        if (strcmp(packet.message, "Success") == 0) {
            return socket_fd;
        }

        close(socket_fd);
        return -1;
    }

    int parse_user_command(int connection_socket) {
        char input[MAX_COMMAND_LEN];
        fgets(input, MAX_COMMAND_LEN, stdin);

        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        if (strcmp(input, "exit") == 0) {
            // Notify server
            subscription_packet packet{};

            sprintf(packet.message, "Quit");
            packet.length = strlen(packet.message);

            connection::send_full_message(connection_socket, (void *)&packet, sizeof(packet));

            close(connection_socket);   // close socket

            exit(0);
        }

        if (strcmp(connection::get_command(input), "subscribe") == 0) {
            /* Send request to server */
            subscription_packet packet{};

            packet.length = strlen(input);
            strncpy(packet.message, input, packet.length);

            connection::send_full_message(connection_socket, (void *)&packet, sizeof(packet));

            /* Await confirmation */
            connection::receive_full_message(connection_socket, (void *)&packet, sizeof(packet));

            if (strcmp(packet.message, "Success") == 0) {
                fprintf(stdout, "Subscribed to topic %s\n", connection::get_topic(input));
                return 0;
            }

            fprintf(stderr, "Subscription to topic %s gone wrong", connection::get_topic(input));
        }

        if (strcmp(connection::get_command(input), "unsubscribe") == 0) {
            /* Send request to server */
            subscription_packet packet{};

            packet.length = strlen(input);
            strncpy(packet.message, input, packet.length);

            connection::send_full_message(connection_socket, (void *)&packet, sizeof(packet));

            /* Await confirmation */
            connection::receive_full_message(connection_socket, (void *)&packet, sizeof(packet));

            if (strcmp(packet.message, "Success") == 0) {
                fprintf(stdout, "Unsubscribed from topic %s\n", connection::get_topic(input));
                return 0;
            }

            fprintf(stderr, "Unsubscribe to topic %s gone wrong", connection::get_topic(input));
            return 0;
        }

        /* Unlisted command */
        fprintf(stderr, "Unlisted command; usage = subscribe <TOPIC> / unsubscribe <TOPIC>.\n");
        return 0;
    }

    /**
     * Connects subscriber to server and parse given commands to send.
     *
     * @param client_id
     * @param ip_server
     * @param PORT
     */
    void login_subscriber(char *client_ID, uint32_t ip_server, const uint16_t PORT) {
        /* Connect to server */
        int socket_fd = connect_to_server(ip_server, PORT, client_ID);
        DIE(socket_fd < 0, "Client was already logged in");

        /* Create poll array for server connections and STDIN commands */
        std::vector<struct pollfd> poll_fds;
        int num_sockets = 2;

        /* Add STDIN */
        poll_fds.push_back({0, POLLIN});

        /* Add server-connection socket */
        poll_fds.push_back({socket_fd, POLLIN});

        while (true) {
            int rc = poll(&poll_fds[0], num_sockets, -1);
            DIE (rc < 0, "Polling error");

            for (int index = 0; index < poll_fds.size(); index++) {
                /* Check if there's data to be read on one of the sockets */
                if (poll_fds[index].revents & POLLIN) {
                    if (poll_fds[index].fd == 0) {  // STDIN command
                        if (parse_user_command(socket_fd) == 1) {   // Exit command
                            return;
                        }
                        continue;
                    }

                    // Received message from server
                    subscription_packet packet;

                    connection::receive_full_message(socket_fd, (void *)&packet, sizeof(packet));

                    if (strcmp(packet.message, "Quit") == 0) {
                        close(poll_fds[index].fd);
                        return;
                    }

                    fprintf(stdout, "%s\n", packet.message);    // Print message
                }

            }
        }
    }
}

#endif
