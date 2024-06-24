#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "helpers.h"

// /**
//  * Description of a TCP Client.
//  */
// struct TCP_Client {
// // public:
// 	char ID[MAX_ID_LEN];
// 	int socket;
// 	char *ip_addr;
// 	uint16_t port;
// 	std::vector<char *> followed_topics;
// };
	// TCP_Client() {}
	// TCP_Client(char *ID, int socket, char *ip, uint16_t port) {
	// 	strncpy(this->ID, ID, MAX_ID_LEN);
	// 	this->socket = socket;
	// 	strncpy(this->ip_addr, ip, MAX_IP_LEN);
	// 	this->port = port;
	// }

	// /**
	//  * Subscribes TCP-Client to given topic.
	//  *
	//  * @param topic
	//  */
	// void subscribe_to_topic(std::string topic,
	// 						std::unordered_map<std::string, std::unordered_set<TCP_Client>>& subscriptions) {
	// 	// Check if given topic has already been added to subscription map
	// 	auto iter = subscriptions.find(topic);

	// 	if (iter != subscriptions.end()) {
	// 		// Check if client is already subscribed to topic
	// 		if (subscriptions[topic].find(*this) != subscriptions[topic].end()) {
	// 			fprintf(stderr, "Client is already subscribed to %s", topic.c_str());
	// 			return;
	// 		}

	// 		// Otherwise, add the client to the subscriber set
	// 		iter->second.insert(*this);
	// 		return;
	// 	}

	// 	// Add topic to map; create new set of subscribers
	// 	subscriptions.emplace(topic, std::unordered_set<TCP_Client>{*this});

	// 	return;
	// }

	// bool operator==(const TCP_Client& other) const {
	// 	if (this->ID == other.ID) {
	// 		return true;
	// 	}

	// 	return false;
	// }        
// };

#endif