#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

#include "consts.h"

char *sock_path;
int client_sock_fd;

void connect_to_sock(char *path) {
	// if (fopen(path, "r") == NULL) {
	// 	puts("nope");
	// 	return;
	// }                  (for some reason fopen returns null when file exists)

	client_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un sockaddr = {0};

	if (client_sock_fd == -1) {
		return;
	}

	int buffern = ioctl(client_sock_fd, FIONREAD);

	sockaddr.sun_family = AF_UNIX;
	strcpy(sockaddr.sun_path, sock_path);

	int ret = connect(client_sock_fd, (struct sockaddr*) &sockaddr, sizeof(struct sockaddr_un));
	if (ret == -1) {
		fprintf(stderr, "Error: No server found running with specified ID\n");
		// return;
		exit(1);
	}
}

int main(int argc, char** argv) {
	if (argc <= 1) {
		fprintf(stderr, "Error: ID is not specified.\n");
		return 1;
	}

	if (argc <= 2) {
		fprintf(stderr, "Error: Command is not specified.\n");
		return 1;
	}

	if (strtol(argv[1], NULL, 10) < 0 || strtol(argv[1], NULL, 10) > 2147483647) {
		fprintf(stderr, "Error: ID must be between 0 and 2147483647\n");
	}

	char *runtime_dir = getenv("XDG_RUNTIME_DIR");

	if (runtime_dir == NULL) {
		fprintf(stderr, "Error: Environment variable XDG_RUNTIME_DIR is NULL, quitting.\n");
		return 1;
	}

	asprintf(&sock_path, SOCK_PATH_FORMAT, runtime_dir, (int)strtol(argv[1], NULL, 10));

	connect_to_sock(sock_path);

	char* buffer = argv[2];

	if (strlen(buffer) > BUFFER_SIZE) {
		fprintf(stderr, "Warning: Command (%lu characters long) is longer than %d characters.\n", strlen(buffer), BUFFER_SIZE);
	}

	write(client_sock_fd, buffer, strlen(buffer) + 1);

	printf("Queued command '%s' at server with ID %s\n", argv[2], argv[1]);  // queueing wasnt an intended feature (idk how to get the sockets to not queue) but its cool so im keeping it

	return 0;
}
