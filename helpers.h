#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <cmath>

#ifndef HELPERS_H
#define HELPERS_H

#define MAX_POLL_FDS 32
#define MAX_COMMAND_LEN 256
#define MAX_ID_LEN 10
#define MAX_IP_LEN 20
#define MAX_NOTIFICATION_LEN 2000

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

namespace connection {
    /**
     * Get command keyword from STDIN buffer.
     *
     * @param buffer
     * @return
     */
    char *get_command(char *buffer) {
        char backup[MAX_COMMAND_LEN];

        strncpy(backup, buffer, MAX_COMMAND_LEN);

        char *p = strtok(backup, " ");

        if (p[strlen(p) - 1] == ' ' || p[strlen(p) - 1] == '\n') {
            p[strlen(p) - 1] = '\0';
        }

        return p;
    }

    /**
     * Get topic from STDIN message.
     *
     * @param buffer
     * @return
     */
    char *get_topic(char *buffer) {
        char *backup = (char *)calloc(MAX_COMMAND_LEN, sizeof(char));

        strncpy(backup, buffer, MAX_COMMAND_LEN);
        
        char *p = strtok(backup, " ");
        p = strtok(NULL, " ");

        return p;
    }

    /**
     * Receive full message from socket-descriptor argument.
     *
     * @param socket socket to receive data from
     * @param buffer message buffer
     * @param len length of message
     * @param isUDP
     * @return number of bytes received
     */
    size_t receive_full_message(int socket, void *buffer, size_t len,
                                    bool isUDP = false, struct sockaddr *from = NULL, socklen_t *addrlen = NULL) {
        size_t bytes_received = 0;
        size_t bytes_remaining = len;
        char *buff = (char *) buffer;

        if (socket == 0) {  // Use fgets() for STDIN
            char *rc = {};

            rc = fgets(buff, len, stdin);
            DIE(rc == NULL, "Error reading from STDIN");

            return len;
        }

        if (isUDP) {    // UDP message - use recvfrom()
            size_t current_byte_count = recvfrom(socket, buff, len, 0, from, addrlen);
            DIE(current_byte_count == -1, "Receive bytes error");

            return len;
        }

        while (bytes_remaining != 0) {
            size_t current_byte_count = recv(socket, buff + bytes_received, bytes_remaining, 0);
            DIE(current_byte_count == -1, "Receive bytes error");

            bytes_remaining -= current_byte_count;
            bytes_received += current_byte_count;
        }

        return bytes_received;
    }

    /**
     * Send full message to socket-descriptor argument.
     *
     * @param socket socket to send data to
     * @param buffer message buffer
     * @param len length of message
     * @return number of bytes sent
     */
    size_t send_full_message(int socket, void *buffer, size_t len) {
        size_t bytes_sent = 0;
        size_t bytes_remaining = len;
        char *buff = (char *) buffer;

        while (bytes_remaining != 0) {
            size_t current_byte_count = send(socket, buff + bytes_sent, bytes_remaining, 0);
            DIE(current_byte_count == -1, "Send bytes error");

            bytes_remaining -= current_byte_count;
            bytes_sent += current_byte_count;
        }

        return bytes_sent;
    }
}

#endif
