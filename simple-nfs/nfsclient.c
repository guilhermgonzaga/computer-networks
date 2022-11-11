// cc: gcc -g -pedantic -Wall -Wextra -o nfsclient nfsclient.c
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"


static char buffer[NFS_BUFSZ] = {0};


int parse_args(int argc, const char *argv[], enum nfs_cmd *cmd, const char **cpath, const char **spath);

int request(int sockfd, enum nfs_cmd cmd, const char *spath, const char *cpath);

int send_data(int sockfd, const char *filepath);

int recv_response(int sockfd, enum nfs_cmd cmd);


int main(int argc, const char *argv[]) {
	const char *cpath, *spath;
	enum nfs_cmd cmd;

	if (EXIT_FAILURE == parse_args(argc, argv, &cmd, &cpath, &spath)) {
		fprintf(stderr,
			"Request operations on remote file system.\n"
			"Usage:\n"
			"  nfsclient (list|create|delete) <spath>\n"
			"  nfsclient upload <cpath> <spath>\n"
			"Where\n"
			"  spath  is the pathname to be used by the server (max. %d chars)."
			"         To create a directory, *create* is specified and the path"
			"         must end in a forward slash.\n"
			"  cpath  refers to the file to be sent to the server when *upload*"
			"         is specified (max. %d chars).\n",
			NFS_PATH_MAX-1, NFS_PATH_MAX-1);
		return EXIT_FAILURE;
	}

	struct sockaddr_in client_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(NFS_PORT)
	};
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	FAIL_IF(-1 == sockfd, "Failed to open socket");

	int status = connect(sockfd, (struct sockaddr *)&client_addr, sizeof client_addr);

	if (EXIT_SUCCESS == status) {
		status = request(sockfd, cmd, spath, cpath);
		if (EXIT_SUCCESS == status) {
			status = recv_response(sockfd, cmd);
		}
	}

	close(sockfd);
	return status;
}


int parse_args(int argc, const char *argv[], enum nfs_cmd *cmd, const char **cpath, const char **spath) {
	*cmd = strtocmd(argv[1]);

	if (3 == argc && (NFS_LIST == *cmd || NFS_CREATE == *cmd || NFS_DELETE == *cmd)) {
		*cpath = NULL;
		*spath = argv[2];
		return EXIT_SUCCESS;
	}

	if (4 == argc && NFS_UPLOAD_FILE == *cmd) {
		*cpath = argv[2];
		*spath = argv[3];
		return EXIT_SUCCESS;
	}

	*cpath = NULL;
	*spath = NULL;
	return EXIT_FAILURE;
}

int request(int sockfd, enum nfs_cmd cmd, const char *spath, const char *cpath) {
	size_t spath_len = strlen(spath) + 1;

	buffer[0] = cmd;
	strcpy(&buffer[1], spath);

	FAIL_IF(-1 == write(sockfd, buffer, 1 + spath_len), "Failed to send command to server");

	if (NFS_UPLOAD_FILE == cmd) {
		struct stat stbuf;
		stat(cpath, &stbuf);

		FAIL_IF(!S_ISREG(stbuf.st_mode) || EXIT_FAILURE == send_data(sockfd, cpath),
		        "Failed to upload file to server");
	}

	return EXIT_SUCCESS;
}

int send_data(int sockfd, const char *filepath) {
	_Bool failed = 0;
	size_t len = 0;
	FILE *data = fopen(filepath, "rb");

	if (NULL == data)
		return EXIT_FAILURE;

	while (!failed && (len = fread(buffer, 1, NFS_BUFSZ, data))) {
		failed = -1 == send(sockfd, buffer, len, (NFS_BUFSZ == len) ? MSG_MORE : 0);
	}

	fclose(data);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

int recv_response(int sockfd, enum nfs_cmd cmd) {
	int resplen = read(sockfd, buffer, NFS_BUFSZ);
	FAIL_IF(resplen <= 0, "Failed to get response from server");

	if (EXIT_FAILURE == buffer[0]) {
		printf("Response: FAILURE\n");
	}
	else if (NFS_LIST == cmd) {
		printf("%.*s", NFS_BUFSZ - 1, &buffer[1]);
	}
	else {
		printf("Response: SUCCESS\n");
	}

	return EXIT_SUCCESS;
}
