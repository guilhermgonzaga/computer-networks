// gcc -Wall -o client udpclient.c
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BUFLEN 1024

int send_wait_msg(int sockfd, char *buffer, size_t msglen, struct sockaddr *servaddr) {
	sendto(sockfd, buffer, msglen, MSG_CONFIRM,
	       servaddr, (socklen_t) sizeof(struct sockaddr_in));

	socklen_t len = sizeof(struct sockaddr_in);  // in-out variable
	int resplen = recvfrom(sockfd, buffer, BUFLEN, MSG_WAITALL, servaddr, &len);

	if (resplen > 0) {
		printf("received %.*s\n", BUFLEN, buffer);
		return 0;
	}
	else {
		fprintf(stderr, "recvfrom failed\n");
		return 1;
	}
}

int main(int argc, const char *argv[]) {
	char buffer[BUFLEN] = {0};
	struct sockaddr_in servaddr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(6789)
	};
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == sockfd) {
		fprintf(stderr, "Failed to open socket\n");
		return 1;
	}

	// For every line of input,
	while (fgets(buffer, BUFLEN, stdin)) {
		size_t msglen = strnlen(buffer, BUFLEN);

		// Remove newline from input
		if ('\n' == buffer[msglen-1])
			buffer[--msglen] = '\0';

		send_wait_msg(sockfd, buffer, msglen+1, (struct sockaddr *)&servaddr);
	}

	close(sockfd);

	return 0;
}
