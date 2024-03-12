#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>

#include "utils.h"


int main(int argc, char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // verific numarul de argumente
    if (argc != 4) {
        fprintf(stderr, "You need to provide ID, server address and port\n");
        return 0;
    }

    // creez socketul
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error creating socket\n");
        return 1;
    }
    
    // setez adresa serverului
    struct sockaddr_in address_server;
    memset((char*) &address_server, 0, sizeof(address_server));
    address_server.sin_family = AF_INET;
    address_server.sin_port = htons(atoi(argv[3]));
    address_server.sin_addr.s_addr = inet_addr(argv[2]);

    // setez ID-ul
    char id[10];
    strcpy(id, argv[1]);

    // conectez socketul la server
    int connect_status = connect(sockfd, (struct sockaddr*) &address_server, sizeof(address_server));
    if (connect_status < 0) {
        fprintf(stderr, "Error connecting to server\n");
        return 1;
    }

    // trimit ID-ul la server
    struct message msg;
    msg.message_type = ID_MSG;
    memcpy(msg.payload, id, 10);
    int return_val = send_all(sockfd, &msg, sizeof(msg));
    if (return_val < 0) {
        fprintf(stderr, "Error sending ID\n");
        return 1;
    }

    // primeste confirmare de la server
    return_val = recv_all(sockfd, &msg, sizeof(msg));
    if (return_val < 0) {
        fprintf(stderr, "Error receiving confirmation\n");
        return 1;
    }

    // verifica daca ID-ul este deja conectat
    if (msg.message_type == EXIT_MSG) {
        fprintf(stderr, "Client %s already connected.\n", id);

        // inchide socketul
        close(sockfd);

        return 1;
    }

    // creez epoll
    int epollfd = epoll_create1(0);

    // adaugam citirea de la tastatura in epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &event);

    // adaugam citirea de la server in epoll
    struct epoll_event event2;
    event2.events = EPOLLIN;
    event2.data.fd = sockfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event2);

    while (true) {
        // asteapta evenimente prin epoll
        struct epoll_event events[2];
        int nr_events = epoll_wait(epollfd, events, 2, -1);
        if (nr_events < 0) {
            fprintf(stderr, "Error waiting for events\n");
            return 1;
        }

        // parcurgem evenimentele
        for (int i = 0; i < nr_events; i++) {
            // verificam daca s-a primit mesaj de la tastatura
            if (events[i].data.fd == STDIN_FILENO) {
                // citim comanda
                char command[100];
                fgets(command, 100, stdin);
                command[strlen(command) - 1] = '\0';

                // verificam daca s-a primit comanda de exit
                if (strcmp(command, "exit") == 0) {
                    // trimitem mesaj de exit la server
                    msg.message_type = EXIT_MSG;
                    return_val = send_all(sockfd, &msg, sizeof(msg));
                    if (return_val < 0) {
                        fprintf(stderr, "Error sending exit message\n");
                        return 1;
                    }

                    return 0;
                } else {
                    // parsam comanda
                    char* token = strtok(command, " ");

                    // verificam daca s-a primit comanda de subscribe
                    if (strcmp(token, "subscribe") == 0) {
                        // parsam topicul
                        token = strtok(NULL, " ");
                        char topic[100];
                        strcpy(topic, token);

                        if (strlen(topic) > 50) {
                            fprintf(stderr, "Topic too long\n");
                            continue;
                        }

                        // parsam SF-ul
                        int sf;
                        token = strtok(NULL, " ");
                        
                        if (token == NULL) {
                            sf = 0;
                        } else {
                            sf = atoi(token);
                        }


                        // trimitem mesaj de subscribe la server
                        msg.message_type = SUBSCRIBE_MSG;
                        msg.store_forward = sf;
                        memcpy(msg.payload, topic, 50);
                        return_val = send_all(sockfd, &msg, sizeof(msg));
                        if (return_val < 0) {
                            fprintf(stderr, "Error sending subscribe message\n");
                            return 1;
                        }

                        // afisam mesajul
                        printf("Subscribed to topic.\n");
                    } else if (strcmp(token, "unsubscribe") == 0) {
                        // parsam topicul
                        token = strtok(NULL, " ");
                        char topic[50];
                        strcpy(topic, token);

                        // trimitem mesaj de unsubscribe la server
                        msg.message_type = UNSUBSCRIBE_MSG;
                        memcpy(msg.payload, topic, 50);
                        return_val = send_all(sockfd, &msg, sizeof(msg));
                        if (return_val < 0) {
                            fprintf(stderr, "Error sending unsubscribe message\n");
                            return 1;
                        }

                        // afisam mesajul
                        printf("Unsubscribed from topic.\n");
                    } else {
                        fprintf(stderr, "Incorrect command\n");
                    }
                }
            } else {
                // s-a primit mesaj de la server (UDP / exit)
                struct message msg;
                return_val = recv_all(sockfd, &msg, sizeof(msg));

                // verificam daca s-a primit mesaj
                if (return_val < 0) {
                    fprintf(stderr, "Error receiving message\n");
                    return 1;
                }

                // verificam daca serverul a inchis conexiunea
                if (msg.message_type == EXIT_MSG || return_val == 0) {
                    fprintf(stderr, "Server closed connection\n");
                    return 0;
                }

                if (msg.message_type == UDP_MSG) {
                    // afisam topicul
                    printf("%s -", msg.topic);

                    // afisam tipul de date
                    if (msg.data_type == 0) {
                        printf(" INT -");
                    } else if (msg.data_type == 1) {
                        printf(" SHORT_REAL -");
                    } else if (msg.data_type == 2) {
                        printf(" FLOAT -");
                    } else if (msg.data_type == 3) {
                        printf(" STRING -");
                    }

                    // afisam continutul
                    if (msg.data_type == 0) {
                        // cast la integer
                        integer_type data;
                        memcpy(&data, msg.data, sizeof(integer_type));

                        if (data.sign == 1) {
                            printf(" -");
                        } else {
                            printf(" ");
                        }

                        printf("%u\n", ntohl(data.value));

                    } else if (msg.data_type == 1) {
                        // cast la short real
                        short_real_type data;
                        memcpy(&data, msg.data, sizeof(short_real_type));

                        printf(" %.2f\n", (float)(ntohs(data.value)) / 100.0f);
                    } else if (msg.data_type == 2) {
                        // cast la float
                        float_type data;
                        memcpy(&data, msg.data, sizeof(float_type));

                        if (data.sign == 1) {
                            printf(" -");
                        } else {
                            printf(" ");
                        }

                        uint8_t precision = data.power;
                        float val = ntohl(data.value);

                        // calculam 10 ^ -precizie zecimale
                        while (precision > 0) {
                            val /= 10.0f;
                            precision--;
                        }

                        printf("%f\n", val);

                    } else if (msg.data_type == 3) {
                        // afisam stringul
                        printf(" %s\n", msg.data);
                    }
                }
            }
        }
    }

    return 0;
}