#ifndef SUBSCRIPTION_PROTOCOL_H
#define SUBSCRIPTION_PROTOCOL_H

#include "helpers.h"
#include "tcp_client.h"

#define MAX_TOPIC_SIZE 50
#define MAX_UDP_PAYLOAD_SIZE 1500

namespace subscription_protocol {
    /**
     * Structure describing a packet sent by a UDP client.
     */
    struct udp_packet {
        char topic[MAX_TOPIC_SIZE];
        uint8_t data_type;
        char payload[MAX_UDP_PAYLOAD_SIZE];
    };

    /**
     * Structure describing a confirmation packet sent by a TCP client.
     */
    struct subscription_packet {
        char message[MAX_NOTIFICATION_LEN];
        size_t length;
    };

    /**
     * Description of a TCP Client.
     */
    struct TCP_Client {
        char ID[MAX_ID_LEN];
        int socket;
        bool isActive;
        char ip_addr[MAX_IP_LEN];
        uint16_t port;
        std::vector<char *> followed_topics;

        bool operator==(const struct TCP_Client &other){
            if(strcmp(ID, other.ID) == 0) {
                return true;
            }

            return false;
        }
    };

    struct hasher {
        std::size_t operator()(const struct TCP_Client& s1) const noexcept
        {
            return std::hash<std::string>{}(s1.ID);
        }
    };

    /**
     * Checks if client with designated ID has already logged in.
     *
     * @param ID
     * @param registered_clients
     * @return socket
     */
    int isRegistered(char *ID, std::unordered_map<int, struct TCP_Client *> registered_clients) {
        for (auto& iter: registered_clients) {
            struct TCP_Client *client = iter.second;
            
            if (strcmp(client->ID, ID) == 0) {
                return iter.first;
            }
        }

        return -1;
    }

    /**
	 * Subscribes TCP-Client to given topic.
	 *
	 * @param topic
     * @param subscriptions
	 */
	void subscribe_to_topic(struct TCP_Client *client, std::string topic,
							std::unordered_map<std::string, std::vector<struct TCP_Client *>>& subscriptions) {
		// Check if given topic has already been added to subscription map
		auto iter = subscriptions.find(topic);

		if (iter != subscriptions.end()) {
			// Check if client is already subscribed to topic
			if (std::find(subscriptions[topic].begin(), subscriptions[topic].end(), client) != subscriptions[topic].end()) {
				fprintf(stderr, "Client is already subscribed to %s", topic.c_str());
				return;
			}

			// Otherwise, add the client to the subscriber set
			iter->second.push_back(client);
			return;
		}

		// Add topic to map; create new set of subscribers
		subscriptions.emplace(topic, std::vector<struct TCP_Client *>{client});

		return;
	}

    /**
	 * Unsubscribes TCP-Client from topics that match given parameter.
	 *
	 * @param topic_wildcard
     * @param subscriptions
	 */
	void unsubscribe_from_topic(struct TCP_Client *client, std::string topic_wildcard,
							std::unordered_map<std::string, std::vector<struct TCP_Client *>>& subscriptions) {    
        // Escape '*' and '+' characters from wildcard
        if (topic_wildcard != std::string{".*"}) {
            topic_wildcard = std::regex_replace(topic_wildcard, std::regex("\\*"), "\\*");
        }
        topic_wildcard = std::regex_replace(topic_wildcard, std::regex("\\+"), "\\+");
            
        // Search topics that match given wildcard and unsubscribe from them
		for (auto& entry: subscriptions) {
            if (std::regex_match(entry.first.c_str(), std::regex(topic_wildcard))) {
                // Unsubscribe (remove from vector of clients)
                auto iter = std::find(entry.second.begin(), entry.second.end(), client);

                if (iter != entry.second.end()) {   // If we found the client
                    entry.second.erase(iter);
                }
            }
        }
    }

    /**
     * Gets notification string.
    */
    char *format_notification(char *ip, uint16_t port, struct udp_packet packet) {
        std::string notification;

        notification.append(packet.topic)
                    .append(" - ");

        // Extract data type identifier and interpret payload value
        char data_type[MAX_COMMAND_LEN];
        char *payload;

        switch (packet.data_type) {
            case 0: {
                sprintf(data_type, "INT");
                notification.append(data_type)
                            .append(" - ");

                payload = (char *)packet.payload;

                char sign = payload[0];

                payload++;
                uint32_t value = *(uint32_t *)payload;

                if (ntohl(value) != 0 && sign == 1) {
                    notification.append("-");
                }

                notification.append(std::to_string(ntohl(value)));

                break;
            }
            case 1: {
                sprintf(data_type, "SHORT_REAL");
                notification.append(data_type)
                            .append(" - ");

                payload = (char *)packet.payload;

                uint16_t value = ntohs(*(uint16_t *)payload);

                char formatted_value[MAX_COMMAND_LEN];

                sprintf(formatted_value, "%.2f", (float)value / 100);

                notification.append(formatted_value);

                break;
            }
            case 2: {
                sprintf(data_type, "FLOAT");
                notification.append(data_type)
                            .append(" - ");

                payload = (char *)packet.payload;

                if (payload[0] == 1) {
                    notification.append("-");
                }

                payload++;
                uint32_t whole_value = ntohl(*(uint32_t *)payload);
                uint8_t power = *((uint8_t *)((uint32_t *)payload + 1));

                char formatted_value[MAX_COMMAND_LEN];

                sprintf(formatted_value, "%.*f", power, (float)whole_value / pow(10, power));

                notification.append(formatted_value);

                break;
            }
            case 3: {
                sprintf(data_type, "STRING");
                notification.append(data_type)
                            .append(" - ");

                payload = (char *)packet.payload;

                notification.append(payload);

                break;
            }
        }

        char *result = (char *) calloc(MAX_NOTIFICATION_LEN, sizeof(char));
        DIE(result == NULL, "Allocation error");

        sprintf(result, "%s", notification.c_str());
                
        return result;
    }

    /**
     * Goes through topic mapping and notifies subscribers about given message.
    */
    void notify_subscribers(std::string new_topic, const char *notification,
							    std::unordered_map<std::string, std::vector<struct TCP_Client *>>& subscriptions) {
        // Create set of sockets we've notified so we don't notify the same client twice
        std::unordered_set<int> notified_sockets;

        for (auto& entry: subscriptions) {
            // Replace '*' and '+' wildcards with regular expressions
            std::string topic = entry.first;

            topic = std::regex_replace(topic, std::regex("\\*"), ".*");
            topic = std::regex_replace(topic, std::regex("\\+"), "[^/]*");

            // Check if new_topic matches regex designated by current entry
            if (std::regex_match(new_topic, std::regex(topic))) {
                // Notify clients
                subscription_packet packet{};

                sprintf(packet.message, "%s", notification);
                packet.length = strlen(packet.message);

                for (auto& client: entry.second) {
                    // If client has not been notified
                    if (notified_sockets.find(client->socket) == notified_sockets.end()
                        && client->isActive) {
                        connection::send_full_message(client->socket, (void *)&packet, sizeof(packet));
                        notified_sockets.insert(client->socket); // Add client to set
                    }
                }
            }
        }
    }
}

#endif
