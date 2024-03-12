#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <set>
#include <fcntl.h>
#include <cstring>

#include "utils.h"

using namespace std;

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // verificam numarul de argumente
    if (argc < 2)
    {
        std::cerr << "Please provide a port number" << std::endl;
        return -1;
    }

    // creez epoll pentru a monitoriza socketii
    int epollfd = epoll_create1(0);

    // verificam daca epoll a fost creat cu succes
    if (epollfd < 0)
    {
        fprintf(stderr, "Error creating epoll\n");
        return -1;
    }

    // creez socketii pentru udp si tcp
    int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // verificam daca socketii au fost creati cu succes
    if (udp_sockfd < 0 || tcp_sockfd < 0)
    {
        fprintf(stderr, "Error creating sockets\n");
        return -1;
    }

    // configurez socketii pentru a putea fi refolositi
    int optval = 1;
    setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // configurez socketul tcp pentru a nu mai folosi algoritmul Nagle
    setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));

    // creez structura pentru adresa serverului
    struct sockaddr_in server_addr;

    // setez adresa serverului
    server_addr.sin_family = AF_INET;

    // setez portul serverului
    server_addr.sin_port = htons(atoi(argv[1]));

    // setez adresa ip a serverului
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(udp_sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    bind(tcp_sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));

    // verific daca socketii au fost bind-uiti cu succes
    if (udp_sockfd < 0 || tcp_sockfd < 0)
    {
        fprintf(stderr, "Error binding sockets\n");
        return -1;
    }

    // setez socketul tcp sa fie socket de ascultare
    if (listen(tcp_sockfd, 5) < 0)
    {
        fprintf(stderr, "Error listening\n");
        return -1;
    }

    // creez vectorul de clienti
    vector<struct subscriber> subscribers;

    // adaug socketul tcp la epoll
    struct epoll_event event;
    event.data.fd = tcp_sockfd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, tcp_sockfd, &event);

    // adaug socketul udp la epoll
    struct epoll_event event2;
    event2.data.fd = udp_sockfd;
    event2.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, udp_sockfd, &event2);

    // adaug stdin la epoll
    struct epoll_event event3;
    event3.data.fd = STDIN_FILENO;
    event3.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &event3);

    while (true)
    {
        // astept evenimente de la epoll
        struct epoll_event events[MAX_CLIENTS];
        int event_count = epoll_wait(epollfd, events, MAX_CLIENTS, -1);

        // verific daca epoll_wait a fost apelat cu succes
        if (event_count < 0)
        {
            fprintf(stderr, "Error epoll_wait\n");
            return -1;
        }

        // parcurg evenimentele
        for (int i = 0; i < event_count; i++)
        {
            if (events[i].data.fd == STDIN_FILENO)
            {
                // citesc de la tastatura
                char buffer[100];
                fgets(buffer, 100, stdin);
                buffer[strlen(buffer) - 1] = '\0';

                // verific daca s-a dat comanda exit
                if (strcmp(buffer, "exit") == 0)
                {
                    // inchid socketii clientilor
                    for (auto it = subscribers.begin(); it != subscribers.end(); it++)
                    {
                        close(it->socket);
                    }

                    // inchid socketii serverului
                    close(tcp_sockfd);
                    close(udp_sockfd);

                    // inchid epoll
                    close(epollfd);

                    return 0;
                }
            }
            else if (events[i].data.fd == tcp_sockfd)
            {
                // creez structura pentru adresa clientului
                struct sockaddr_in address_client;
                socklen_t address_client_len = sizeof(address_client);

                // accept conexiunea de la client
                int client_sockfd = accept(tcp_sockfd, (struct sockaddr *)&address_client, &address_client_len);

                // verific daca accept a fost apelat cu succes
                if (client_sockfd < 0)
                {
                    fprintf(stderr, "Error accepting connection\n");
                    return 1;
                }
                // configurez socketul clientului pentru a nu mai folosi algoritmul Nagle
                int opt = 1;
                setsockopt(client_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

                // primesc id-ul clientului
                struct message msg;
                int return_val = recv_all(client_sockfd, &msg, sizeof(msg));

                // verific daca recv a fost apelat cu succes
                if (return_val < 0)
                {
                    fprintf(stderr, "Error receiving client id\n");
                    return -1;
                }

                // verific daca id-ul este unic
                bool unique = true;
                bool good_to_go = false;

                for (auto it = subscribers.begin(); it != subscribers.end(); it++)
                {
                    if (strcmp(it->id.c_str(), msg.payload) == 0)
                    {
                        unique = false;
                        if (it->is_active == false)
                        {
                            it->is_active = true;
                            it->socket = client_sockfd;
                            good_to_go = true;

                            // trimit mesaj de confirmare
                            struct message confirmation_msg;
                            confirmation_msg.message_type = ID_MSG;
                            return_val = send_all(client_sockfd, &confirmation_msg, sizeof(confirmation_msg));
                            if (return_val < 0)
                            {
                                fprintf(stderr, "Error sending confirmation message\n");
                                return -1;
                            }
                        }
                    }
                }

                if (unique == false && good_to_go == false)
                {
                    // trimit mesaj de eroare (clientul este deja conectat)
                    struct message error_msg;
                    error_msg.message_type = EXIT_MSG;
                    return_val = send_all(client_sockfd, &error_msg, sizeof(error_msg));
                    if (return_val < 0)
                    {
                        fprintf(stderr, "Error sending error message\n");
                        return -1;
                    }

                    // afisez mesaj de eroare
                    fprintf(stderr, "Client %s is already connected\n", msg.payload);
                    continue;
                }

                if (unique == true)
                {
                    // adaug clientul in vectorul de clienti
                    struct subscriber new_subscriber;
                    new_subscriber.id = msg.payload;
                    new_subscriber.socket = client_sockfd;
                    new_subscriber.is_active = true;
                    subscribers.push_back(new_subscriber);

                    // trimit mesaj de confirmare
                    struct message confirmation_msg;
                    confirmation_msg.message_type = ID_MSG;
                    return_val = send_all(client_sockfd, &confirmation_msg, sizeof(confirmation_msg));
                    if (return_val < 0)
                    {
                        fprintf(stderr, "Error sending confirmation message\n");
                        return -1;
                    }
                }

                // adaug socketul clientului la epoll
                struct epoll_event event;
                event.data.fd = client_sockfd;
                event.events = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sockfd, &event);

                // afisez mesaj de conectare
                printf("New client %s connected from %s:%d\n", msg.payload, inet_ntoa(address_client.sin_addr), ntohs(address_client.sin_port));
            }
            else if (events[i].data.fd == udp_sockfd)
            {
                // creez structura pentru adresa clientului
                struct udp_msg topic_message;
                struct sockaddr_in address_client;
                socklen_t address_client_len = sizeof(address_client);

                // primesc mesajul de la client prin UDP (topic + data_type + payload)
                int return_val = recvfrom(udp_sockfd, &topic_message, sizeof(topic_message), MSG_WAITALL, (struct sockaddr *)&address_client, &address_client_len);
                if (return_val < 0)
                {
                    fprintf(stderr, "Error receiving udp message\n");
                    return -1;
                }

                // caut clientii abonati la topicul respectiv si le trimit mesajul
                for (auto it = subscribers.begin(); it != subscribers.end(); it++)
                {
                    for (auto it2 = it->topics.begin(); it2 != it->topics.end(); it2++)
                    {
                        if (strcmp(it2->c_str(), topic_message.topic) == 0 && it->is_active == true)
                        {
                            // creez mesajul de trimis
                            struct message msg;
                            msg.message_type = UDP_MSG;

                            msg.data_type = topic_message.data_type;
                            strcpy(msg.topic, topic_message.topic);
                            memcpy(msg.data, topic_message.payload, 1500);

                            // trimit mesajul
                            return_val = send_all(it->socket, &msg, sizeof(msg));
                            if (return_val < 0)
                            {
                                fprintf(stderr, "Error sending message\n");
                                return -1;
                            }
                            break;
                        }
                    }
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                // primesc mesajul de la client prin TCP
                struct message tcp_message;
                int return_val = recv_all(events[i].data.fd, &tcp_message, sizeof(tcp_message));

                // verific daca recv a fost apelat cu succes
                if (return_val < 0)
                {
                    fprintf(stderr, "Error receiving message\n");
                    return -1;
                }

                string id = "";

                // verific daca mesaajul este de exit
                if (tcp_message.message_type == EXIT_MSG)
                {
                    // caut id-ul clientului si il marchez ca inactiv
                    for (auto it = subscribers.begin(); it != subscribers.end(); it++)
                    {
                        if (it->socket == events[i].data.fd)
                        {
                            id = it->id;
                            it->is_active = false;
                            it->socket = -1;
                            break;
                        }
                    }

                    // inchid socketul
                    close(events[i].data.fd);

                    // scot socketul din epoll
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);

                    // afisez mesajul de deconectare
                    printf("Client %s disconnected.\n", id.c_str());
                }

                // verific daca mesajul este de subscribe
                if (tcp_message.message_type == SUBSCRIBE_MSG)
                {
                    // verific daca clientul este activ
                    bool already_subscribed = false;

                    for (auto it = subscribers.begin(); it != subscribers.end(); it++)
                    {
                        if (it->socket == events[i].data.fd)
                        {
                            for (long unsigned int j = 0; j < it->topics.size(); j++)
                            {
                                if (it->topics[j] == string(tcp_message.payload))
                                {
                                    already_subscribed = true;
                                    // afisez mesajul de eroare
                                    fprintf(stderr, "Already subscribed\n");

                                    break;
                                }
                            }
                        }
                    }

                    // adaug topicul la client
                    if (!already_subscribed)
                    {
                        for (auto it = subscribers.begin(); it != subscribers.end(); it++)
                        {
                            if (it->socket == events[i].data.fd)
                            {
                                // afisez mesajul de subscribe
                                fprintf(stderr, "Client %s subscribed to topic %s\n", it->id.c_str(), tcp_message.payload);

                                it->topics.push_back(string(tcp_message.payload));
                                break;
                            }
                        }
                    }
                }

                // verific daca mesajul este de unsubscribe
                if (tcp_message.message_type == UNSUBSCRIBE_MSG)
                {
                    // caut topicul si il sterg
                    for (auto it = subscribers.begin(); it != subscribers.end(); it++)
                    {
                        if (it->socket == events[i].data.fd)
                        {
                            for (long unsigned int j = 0; j < it->topics.size(); j++)
                            {
                                if (it->topics[j] == string(tcp_message.payload))
                                {
                                    // afisez mesajul de unsubscribe
                                    fprintf(stderr, "Client %s unsubscribed from topic %s\n", it->id.c_str(), tcp_message.payload);

                                    it->topics.erase(it->topics.begin() + j);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // afisez mesajul de eroare
                fprintf(stderr, "Invalid event\n");
            }
        }
    }

    // inchid socketii
    close(tcp_sockfd);
    close(udp_sockfd);
    close(epollfd);

    return 0;
}
