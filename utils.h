#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

using namespace std;

#define MAX_CLIENTS 500

#define EXIT_MSG 0
#define SUBSCRIBE_MSG 1
#define UNSUBSCRIBE_MSG 2
#define ID_MSG 3
#define UDP_MSG 4

int recv_all(int sockfd, void *buffer, size_t len);
int send_all(int sockfd, void *buffer, size_t len);

// Struct pentru comunicare UDP
struct udp_msg {
    char topic[50];
    uint8_t data_type;
    char payload[1500];
} __attribute__((packed));

// Struct pentru comunicare TCP (mesaj Server <-> Client TCP)
struct message {
    uint8_t message_type; // 0 - exit, 1 - subscribe, 2 - unsubscribe, 3 - id, 4 - udp datagram
    uint8_t store_forward;
    char payload[100];
    char topic[50];
    uint8_t data_type;
    char data[1500];
} __attribute__((packed));

// Struct pentru clienti
struct subscriber {
    bool is_active;
    int socket;
    int sf;
    string id;
    vector<string> topics;
};

// Struct-uri pentru tipuri de date
struct integer_type {
    uint8_t sign;
    uint32_t value;
} __attribute__((packed));

struct short_real_type {
    uint16_t value;
} __attribute__((packed));

struct float_type {
    uint8_t sign;
    uint32_t value;
    uint8_t power;
} __attribute__((packed));

#endif