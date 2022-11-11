// cc: gcc -g -pedantic -Wall -Wextra -o nfsserver nfsserver.c
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"

#define NFS_SOCKMAX 5


int handle_connection(int sockfd);

int handle_request(int connfd, char buffer[]);

int list_dir(const char *path, char buffer[]);

int create(const char *path);

int save_to_file(const char *path, int connfd, char buffer[]);

int delete(const char *path);

int delete_recursive(const char *path);


int main(void) {
	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(NFS_PORT)
	};
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	FAIL_IF(-1 == sockfd, "Failed to open socket");

	FAIL_IF(bind(sockfd, (struct sockaddr *)&server_addr, sizeof server_addr),
	         "Failed to bind socket");
	FAIL_IF(listen(sockfd, NFS_SOCKMAX), "Socket listen failed");

	fprintf(stderr, "Server is listening...\n");

	while (1) {
		handle_connection(sockfd);
	}

	close(sockfd);

	return 0;
}


int handle_connection(int sockfd) {
	struct sockaddr_in client_addr = {0};
	char buffer[NFS_BUFSZ] = {0};

	socklen_t len = sizeof client_addr;  // in-out variable
	int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &len);
	FAIL_IF(connfd < 0, "Server accept failed");

	printf("New connection\n");

	int8_t status = EXIT_FAILURE;

	if (-1 != recv(connfd, buffer, sizeof buffer, 0)) {
		status = handle_request(connfd, buffer);
	}

	if (-1 == send(connfd, &status, sizeof status, 0)) {
		status = EXIT_FAILURE;
		fprintf(stderr, "Failed to send response");
	}

	close(connfd);
	printf("Connection closed\n");

	return status;
}

int handle_request(int connfd, char buffer[]) {
	enum nfs_cmd cmd = buffer[0];
	const char *path = buffer + 1;

	printf("Request: %s %s\n", cmdtostr(cmd), path);

	if (cmd == NFS_LIST) {
		if (EXIT_SUCCESS == list_dir(path, buffer)) {
			send(connfd, buffer, 1 + strlen(path) + 1, 0);
			return EXIT_SUCCESS;
		}
	}
	else if (cmd == NFS_CREATE) {
		return create(path);
	}
	else if (cmd == NFS_UPLOAD_FILE) {
		return save_to_file(path, connfd, buffer);
	}
	else if (cmd == NFS_DELETE) {
		return delete(path);
	}

	return EXIT_FAILURE;
}

int list_dir(const char *path, char buffer[]) {
	buffer[0] = EXIT_SUCCESS;
	errno = 0;

	DIR *dir = opendir(path);
	if (dir) {
		struct dirent *dirent;
		int offset = 1;

		while (offset < NFS_BUFSZ && NULL != (dirent = readdir(dir))) {
			if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, "..")) {
				offset += sprintf(buffer + offset, "%.*s\n", NFS_BUFSZ - offset, dirent->d_name);
			}
		}

		if (1 == offset)
			buffer[1] = '\0';

		closedir(dir);
	}

	if (errno) {
		strerror_r(errno, buffer + 1, NFS_BUFSZ - 1);
		buffer[0] = EXIT_FAILURE;
	}

	return buffer[0];
}

int create(const char *path) {
	if ('/' == path[strlen(path) - 1]) {
		return mkdir(path, 0666) ? EXIT_FAILURE : EXIT_SUCCESS;
	}
	else {
		FILE *f = fopen(path, "a");
		return (f && !fclose(f)) ? EXIT_SUCCESS : EXIT_FAILURE;
	}
}

int save_to_file(const char *path, int connfd, char buffer[]) {
	ssize_t len;
	FILE *data = fopen(path, "wb");
	FAIL_IF(NULL == data, "Failed to open file");

	while (0 < (len = recv(connfd, buffer, NFS_BUFSZ, 0))) {
		fwrite(buffer, 1, len, data);
	}

	fclose(data);
	return -1 == len ? EXIT_FAILURE : EXIT_SUCCESS;
}

int delete(const char *path) {
	struct stat stbuf;
	stat(path, &stbuf);
	int status = EXIT_SUCCESS;

	if (S_ISDIR(stbuf.st_mode) && -1 == delete_recursive(path)) {
		status = EXIT_FAILURE;
	}

	if (EXIT_FAILURE != status && -1 == remove(path)) {
		status = EXIT_FAILURE;
	}

	return status;
}

int delete_recursive(const char *path) {
	_Bool failed = 0;
	struct dirent *dirent;
	DIR *dir = opendir(path);

	if (NULL == dir)
		return EXIT_FAILURE;

	char subdir[NFS_PATH_MAX];

	while (!failed && NULL != (dirent = readdir(dir))) {
		if (0 == strcmp(dirent->d_name, ".") || 0 == strcmp(dirent->d_name, ".."))
			continue;

		strcpy(subdir, path);
		strcat(subdir, "/");
		strcat(subdir, dirent->d_name);

		if (DT_DIR == dirent->d_type)
			failed = (EXIT_FAILURE == delete_recursive(subdir));

		if (-1 == remove(subdir))
			failed = 1;
	}

	closedir(dir);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
