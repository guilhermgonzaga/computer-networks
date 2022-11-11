#ifndef COMMON_H
#define COMMON_H


#include <string.h>

#define NFS_PATH_MAX 512
#define NFS_BUFSZ NFS_PATH_MAX
#define NFS_PORT 7890

// TODO use perror or variadic macro
#define FAIL_IF(Err, Msg) \
do if (Err) {fprintf(stderr, "%s\n", Msg); return EXIT_FAILURE;} while(0)

enum nfs_cmd {
	NFS_LIST,
	NFS_CREATE,
	NFS_UPLOAD_FILE,
	NFS_DELETE,
	NFS_INVALID
};

// TODO use standard FTP reply codes
// https://datatracker.ietf.org/doc/html/rfc959#page-37

static inline const char *cmdtostr(enum nfs_cmd cmd) {
	switch (cmd) {
	case NFS_LIST:
		return "list";
	case NFS_CREATE:
		return "create";
	case NFS_UPLOAD_FILE:
		return "upload";
	case NFS_DELETE:
		return "delete";
	default:  /* fallthrough */
	case NFS_INVALID:
		return "(INVALID)";
	}
}

static inline enum nfs_cmd strtocmd(const char *str) {
	if (str) {
		if (0 == strcmp(str, "list"))
			return NFS_LIST;
		else if (0 == strcmp(str, "create"))
			return NFS_CREATE;
		else if (0 == strcmp(str, "upload"))
			return NFS_UPLOAD_FILE;
		else if (0 == strcmp(str, "delete"))
			return NFS_DELETE;
	}

	return NFS_INVALID;
}


#endif // COMMON_H
