#ifndef SERVER_BACKEND_H
#define SERVER_BACKEND_H

#include "helpers.h"
#include "subscription_protocol.h"

using namespace subscription_protocol;

namespace server {
	/**
	 * Creates a new TCP/UDP socket and binds it to given port.
	 *
	 * @param domain
	 * @param type
	 * @return
	 */
	int open_passive_socket(int domain, int type, uint16_t PORT) {
		/* Create passive listening socket */
		const int listen_fd = socket(domain, type, 0);
		DIE(listen_fd < 0, "Passive socket error");

		/* Make socket address reusable */
		const int enable = 1;
		if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			perror("setsockopt(SO_REUSEADDR) failed");

		/* Bind socket to given port */
		struct sockaddr_in server_addr = {};
		memset(&server_addr, 0, sizeof(server_addr));

		server_addr.sin_family = domain;
		server_addr.sin_port = htons(PORT);

		int rc = bind(listen_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
		DIE(rc < 0, "Binding error");

		return listen_fd;
	}

	/**
	 * Accepts a new TCP client connection and adds the resulting socket to poll array.
	 *
	 * @param poll_fds
	 * @param num_sockets
	 */
	void handle_tcp_connection(std::vector<struct pollfd>& poll_fds, int &num_sockets,
			std::unordered_map<int, struct TCP_Client *>& tcp_clients) {
		struct sockaddr_in client_addr = {};
		socklen_t cli_len = sizeof(client_addr);

		const int connection_socket =
				accept(poll_fds[1].fd, (struct sockaddr *)&client_addr, &cli_len);
		DIE(connection_socket < 0, "Connection error");

		/* Check if client has not logged in already
		 * (client will send a confirmation ID for us to check) */
		subscription_packet packet{};

		size_t bytes_received = connection::receive_full_message(connection_socket, (void *)&packet, sizeof(packet));
		DIE(bytes_received < 0, "Login confirmation for TCP client failed");

		char *client_ID = (char *) calloc(MAX_ID_LEN, sizeof(char));
		
		strncpy(client_ID, packet.message, packet.length);

		int clientSocket = isRegistered(client_ID, tcp_clients);

		if (clientSocket != -1) {
			struct TCP_Client *client = tcp_clients[clientSocket];

			if (client->isActive) {
				/* Close connection */
				fprintf(stdout, "Client %s already connected.\n", packet.message);

				sprintf(packet.message, "Quit");
				connection::send_full_message(connection_socket, (void *)&packet, sizeof(packet));    // Send close notification

				close(connection_socket);   // Close socket

				return;
			}

			// Client has come back; reset socket
			client->isActive = true;
			client->socket = connection_socket;

			// Update socket fd in tcp_clients
			auto entry = tcp_clients.extract(clientSocket);
			entry.key() = connection_socket;
			
			tcp_clients.insert(std::move(entry));

			/* Add connection socket to poll descriptor array */
			poll_fds.push_back({connection_socket, POLLIN, 0});
			
			num_sockets++;

			sprintf(packet.message, "Success");
			connection::send_full_message(connection_socket, (void *)&packet, sizeof(packet));

			fprintf(stdout, "New client %s connected from %s:%hu.\n",
				client->ID, client->ip_addr, ntohs(client->port));

			return;
		}

		/* If client connection is new, notify client about success
		 * and create a new TCP_Client instance and add it to our vector*/
		struct TCP_Client *new_client = (struct TCP_Client *) malloc(sizeof(struct TCP_Client));
		DIE(new_client == NULL, "Allocation error");

		sprintf(new_client->ID, "%s", client_ID);
		new_client->socket = connection_socket;
		new_client->isActive = true;
		sprintf(new_client->ip_addr, "%s", inet_ntoa(client_addr.sin_addr));
		new_client->port = client_addr.sin_port;

		/* Add connection socket to poll descriptor array */
		poll_fds.push_back({connection_socket, POLLIN, 0});

		tcp_clients.insert({poll_fds[num_sockets].fd, new_client});

		num_sockets++;

		sprintf(packet.message, "Success");
		connection::send_full_message(connection_socket, (void *)&packet, sizeof(packet));

		fprintf(stdout, "New client %s connected from %s:%hu.\n",
				new_client->ID, new_client->ip_addr, ntohs(new_client->port));
	}

	/**
	 * Process UDP client message.
	*/
    void process_udp_message(std::vector<struct pollfd>& poll_fds,
								std::unordered_map<std::string, std::vector<struct TCP_Client *>>& subscriptions) {
        /* Get UDP packet */
        struct udp_packet packet;
		struct sockaddr_in from;
		socklen_t addrlen = sizeof(struct sockaddr);

        connection::receive_full_message(poll_fds[2].fd,
			(void *)&packet, sizeof(packet), true, (struct sockaddr *)&from, &addrlen);

		char *ip_udp_client = inet_ntoa(from.sin_addr);
		uint16_t port_udp_client = ntohs(from.sin_port);

		char notification[MAX_NOTIFICATION_LEN];

		sprintf(notification, "%s", format_notification(ip_udp_client, port_udp_client, packet));

		notify_subscribers(std::string{packet.topic}, notification, subscriptions);
    }

	/**
	 * Reads user input command and handles request.
	 *
	 * @param poll_fds
	 * @param num_sockets
	 */
	int handle_stdin_command(std::vector<struct pollfd>& poll_fds, int &num_sockets) {
		subscription_packet packet{};

		connection::receive_full_message(0, (void *)&packet, sizeof(packet));

		if (strcmp(connection::get_command(packet.message), "exit") == 0) {
			close(poll_fds[1].fd); // close TCP listening socket
			close(poll_fds[2].fd); // close UDP socket

			/* Close TCP clients */
			for (int index = 3; index < poll_fds.size(); index++) {
				sprintf(packet.message, "Quit");
				connection::send_full_message(poll_fds[index].fd, (void *)&packet, sizeof(packet));    // Send close notification

				close(poll_fds[index].fd);   // Close socket
			}

			num_sockets = 0;

			exit(0);
		}

		fprintf(stderr, "Unlisted command\n");
		return 0;
	}

	/**
	 * Process TCP client requests regarding subscriptions (and not only).
	*/
    void process_client_request(std::vector<struct pollfd>& poll_fds, int index, int& num_sockets,
									std::unordered_map<std::string, std::vector<struct TCP_Client *>>& subscriptions,
										std::unordered_map<int, struct TCP_Client *>& tcp_clients) {
        /* Get client request */
		int socket = poll_fds[index].fd;
        subscription_packet packet{};

        connection::receive_full_message(socket, (void *)&packet, sizeof(packet));

		/* Check if client logged out */
		if (strcmp(packet.message, "Quit") == 0) {
			fprintf(stdout, "Client %s disconnected.\n", tcp_clients[socket]->ID);

			// Turn client inactive
			struct TCP_Client *client = tcp_clients[socket];
			client->isActive = false;

			// Close socket and remove descriptor from poll vector
			close(socket);
			poll_fds.erase(poll_fds.begin() + index);

			num_sockets--;

			return;
		}

        if (strcmp(connection::get_command(packet.message), "subscribe") == 0) {
            char *topic = connection::get_topic(packet.message);

			std::string topic_string(topic);

			/* Get client with specified socket */
			struct TCP_Client *client = tcp_clients[socket];

			/* Subscribe to matching topics */
			subscribe_to_topic(client, topic_string, subscriptions);

			/* Send confirmation to client */
			sprintf(packet.message, "Success");
			connection::send_full_message(socket, (void *)&packet, sizeof(packet));

			return;
        }

        if (strcmp(connection::get_command(packet.message), "unsubscribe") == 0) {
			char *topic = connection::get_topic(packet.message);

			std::string topic_string(topic);

			/* Get client with specified socket */
			struct TCP_Client *client = tcp_clients[socket];

			/* Unsubscribe from matching topics */
			unsubscribe_from_topic(client, topic_string, subscriptions);

			/* Send confirmation to client */
			sprintf(packet.message, "Success");
			connection::send_full_message(socket, (void *)&packet, sizeof(packet));

			return;
        }
    }

	/**
	 * Run server to handle multiple client connections simultaneously
	 * using the poll() multiplexing mechanism
	 *
	 * @param tcp_listen_fd socket listening for tcp connections
	 */
	void run_server(int tcp_listen_fd, int udp_socket) {
		/* Create vector of poll descriptors */
		std::vector<struct pollfd> poll_fds;
		int num_sockets = 3;

		int rc = listen(tcp_listen_fd, MAX_POLL_FDS);
		DIE(rc < 0, "Listening error");

		/* Add stdin descriptor */
		poll_fds.push_back({0, POLLIN, 0});

		/* Add TCP listening socket */
		poll_fds.push_back({tcp_listen_fd, POLLIN, 0});

		/* Add UDP socket */
		poll_fds.push_back({udp_socket, POLLIN, 0});

		/* Create mapping between topics and subscribed TCP clients */
		std::unordered_map<std::string, std::vector<struct TCP_Client *>> subscriptions;

		/* Create Socket <-> TCP_Client map */
		std::unordered_map<int, struct TCP_Client *> tcp_clients;

		while (true) {
			rc = poll(&poll_fds[0], num_sockets, -1);
			DIE (rc < 0, "Polling error");

			for (int index = 0; index < poll_fds.size(); index++) {
				/* Check if there's data to be read on one of the sockets */
				if (poll_fds[index].revents & POLLIN) {
					if (poll_fds[index].fd == tcp_listen_fd) { // TCP client-connection request
						handle_tcp_connection(poll_fds, num_sockets, tcp_clients);
						continue;
					}

					if (poll_fds[index].fd == udp_socket) { // UDP client request
						process_udp_message(poll_fds, subscriptions);
						continue;
					}

					if (poll_fds[index].fd == 0) {  // STDIN command
						if (handle_stdin_command(poll_fds, num_sockets) == 1) { // Exit command
							return;
						}
						continue;
					}

					/* One of the clients has sent new data to process */
					process_client_request(poll_fds, index, num_sockets,
												subscriptions, tcp_clients);
				}
			}
		}

	}
}

#endif
