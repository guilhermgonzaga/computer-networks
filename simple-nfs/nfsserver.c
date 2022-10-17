// cc: gcc -g -pedantic -Wall -Wextra -o nfsserver nfsserver.c
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
	// XXX FAIL_IF(chroot("."), "Could not set server root at current working directory");

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

	printf("New client connection\n");  // DEBUG

	int8_t status = EXIT_FAILURE;

	if (-1 != recv(connfd, buffer, sizeof buffer, 0)) {
		status = handle_request(connfd, buffer);
	}

	if (-1 == send(connfd, &status, sizeof status, 0)) {
		status = EXIT_FAILURE;
		fprintf(stderr, "Failed to send response");  // DEBUG
	}

	close(connfd);
	printf("Client connection closed\n");  // DEBUG

	return status;
}

int handle_request(int connfd, char buffer[]) {
	char path[NFS_PATH_MAX] = {0};
	enum nfs_cmd cmd = buffer[0];

	printf("New request: %s %s\n", cmdtostr(cmd), &buffer[1]);

	if (realpath(&buffer[1], path)) {
		if (cmd == NFS_LIST) {
			if (EXIT_SUCCESS == list_dir(path, buffer)) {
				send(connfd, buffer, strlen(buffer) + 1, 0);
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
	}

	return EXIT_FAILURE;
}

int list_dir(const char *path, char buffer[]) {
	int offset = 0;
	struct dirent *dirent;
	DIR *dir = opendir(path);
	FAIL_IF(NULL == dir, "Failed to open directory");

	while (offset < NFS_BUFSZ && NULL != (dirent = readdir(dir))) {
		if (0 == strcmp(dirent->d_name, ".") || 0 == strcmp(dirent->d_name, ".."))
			continue;

		int len = sprintf(buffer+offset, "%.*s\n", NFS_BUFSZ-offset, dirent->d_name);
		// if (offset + len < NFS_BUFSZ) {
			offset += len;
		// }
		// else {
		// 	send(connfd, buffer, NFS_BUFSZ, 0);
		// 	offset = sprintf(buffer, "%.*s\n", NFS_BUFSZ, dirent->d_name + len);
		// }
	}
	// int status = -1 == send(connfd, buffer, , 0) ? EXIT_FAILURE : EXIT_SUCCESS;

	closedir(dir);
	return EXIT_SUCCESS;
}

int create(const char *path) {
	return mkdir(path, 0666) ? EXIT_FAILURE : EXIT_SUCCESS;
	// TODO remove
	// FILE *fp = fopen(path, "w");
	// fclose(fp);
	// return EXIT_SUCCESS;
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

	if (S_ISDIR(stbuf.st_mode))
		FAIL_IF(-1 == delete_recursive(path), "Failed to delete directory");
	else if (S_ISREG(stbuf.st_mode))
		FAIL_IF(-1 == remove(path), "Failed to delete file");

	return EXIT_SUCCESS;
}

int delete_recursive(const char *path) {
	_Bool failed = 0;
	struct dirent *dirent;
	DIR *dir = opendir(".");

	if (NULL == dir)
		return EXIT_FAILURE;

	while (!failed && NULL != (dirent = readdir(dir))) {  // XXX use readdir_r?
		if (DT_DIR == dirent->d_type &&
		    strcmp(dirent->d_name, ".") &&
		    strcmp(dirent->d_name, "..")) {
			char subdir[NFS_PATH_MAX];
			strcpy(subdir, path);
			strcat(subdir, "/");
			strcat(subdir, dirent->d_name);
			failed = EXIT_FAILURE == delete_recursive(subdir);
		}

		if (-1 == remove(path))
			failed = 1;
	}

	closedir(dir);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
