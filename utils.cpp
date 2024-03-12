#include "utils.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

// functie pentru a primi un mesaj in intregime
int recv_all(int sockfd, void *buffer, size_t len) {
	size_t bytes_received = 0;
	size_t bytes_remaining = len;
	char *buff = (char*)buffer;

	while (bytes_remaining) {
		int rc = recv(sockfd, buff + bytes_received, bytes_remaining, 0);

        if (rc == -1)
            return -1;

		if (rc == 0)
			return bytes_received;

		bytes_received += rc;
		bytes_remaining -= rc;
	}

  return bytes_received;
}

// functie pentru a trimite un mesaj in intregime
int send_all(int sockfd, void *buffer, size_t len) {
  	size_t bytes_sent = 0;
  	size_t bytes_remaining = len;
  	char *buff = (char*)buffer;

  	while (bytes_remaining) {
		int rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);

        if (rc == -1)
            return -1;

	  	if (bytes_sent == 0)
			return bytes_sent;

	  	bytes_sent += rc;
	  	bytes_remaining -= rc;
	}
  
  	return bytes_sent;
}