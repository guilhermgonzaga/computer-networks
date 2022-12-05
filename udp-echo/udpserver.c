// gcc -Wall -o server udpserver.c
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFLEN 1024

int main(int argc, const char *argv[]) {
	char buffer[BUFLEN] = {0};
	struct sockaddr_in servaddr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(6789)
	};
	struct sockaddr_in cliaddr = {0};
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == sockfd) {
		fprintf(stderr, "Failed to open socket\n");
		return 1;
	}

	bind(sockfd, (struct sockaddr *)&servaddr, sizeof servaddr);

	while (1) {
		socklen_t len = sizeof cliaddr;  // in-out variable
		int reqlen = recvfrom(sockfd, buffer, BUFLEN, MSG_WAITALL,
		                      (struct sockaddr *)&cliaddr, &len);
		if (reqlen < 0)
			fprintf(stderr, "recvfrom failed\n");

		printf("Request: (%d) %.*s\n", reqlen, BUFLEN, buffer);
		fflush(stdout);

		sendto(sockfd, buffer, reqlen, MSG_CONFIRM,
		       (struct sockaddr *)&cliaddr, (socklen_t) sizeof cliaddr);
	}

	close(sockfd);

	return 0;
}
