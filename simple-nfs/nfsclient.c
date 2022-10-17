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


int parse_args(int argc, const char *argv[], enum nfs_cmd *cmd, const char **file, const char **path);

int request(int sockfd, enum nfs_cmd cmd, const char *path, const char *file);

int send_data(int sockfd, const char *filepath);

int recv_response(int sockfd, enum nfs_cmd cmd);


int main(int argc, const char *argv[]) {
	const char *file, *path;
	enum nfs_cmd cmd = NFS_INVALID;

	if (EXIT_FAILURE == parse_args(argc, argv, &cmd, &file, &path)) {
		fprintf(stderr,
			"Request operations on remote file system.\n"
			"Usage:\n"
			"  nfsclient (list|create|delete) <path>\n"
			"  nfsclient upload <filepath> <path>\n"
			"Where\n"
			"  path      is the pathname to be accessed on the server (max. %d chars);\n"
			"  filepath  points to the file to be sent to the server (max. %d chars).\n",
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
		status = request(sockfd, cmd, path, file);
		if (EXIT_SUCCESS == status) {
			status = recv_response(sockfd, cmd);
		}
	}

	close(sockfd);
	return status;
}


int parse_args(int argc, const char *argv[], enum nfs_cmd *cmd, const char **file, const char **path) {
	*cmd = strtocmd(argv[1]);

	if (3 == argc && (NFS_LIST == *cmd || NFS_CREATE == *cmd || NFS_DELETE == *cmd)) {
		*file = NULL;
		*path = argv[2];
		return EXIT_SUCCESS;
	}

	if (4 == argc && NFS_UPLOAD_FILE == *cmd) {
		*file = argv[2];
		*path = argv[3];
		return EXIT_SUCCESS;
	}

	*file = NULL;
	*path = NULL;
	return EXIT_FAILURE;
}

int request(int sockfd, enum nfs_cmd cmd, const char *path, const char *file) {
	size_t pathlen = strlen(path) + 1;

	buffer[0] = cmd;
	strcpy(&buffer[1], path);

	FAIL_IF(-1 == write(sockfd, buffer, 1 + pathlen), "Failed to send command to server");

	if (NFS_UPLOAD_FILE == cmd) {
		struct stat stbuf;
		stat(file, &stbuf);

		FAIL_IF(!S_ISREG(stbuf.st_mode) || EXIT_FAILURE == send_data(sockfd, file),
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

// TODO receive FTP reply codes
int recv_response(int sockfd, enum nfs_cmd cmd) {
	int resplen = read(sockfd, buffer, NFS_BUFSZ);
	FAIL_IF(-1 == resplen, "Failed to get response from server");

	if (NFS_LIST == cmd) {
		printf("%.*s", NFS_BUFSZ, buffer);
	}
	else if (resplen == 1) {
		printf("Received: %s\n", EXIT_SUCCESS == buffer[0] ? "SUCCESS" : "FAILURE");  // DEBUG
	}

	return EXIT_SUCCESS;
}
