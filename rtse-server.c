#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>

#include "consts.h"
#include "strcheck.h"

char *sock_path;
int server_sock_fd;
int accept_sock_fd;

char *id;

void cleanup() {
	remove(sock_path);
	exit(0);
}

void randomize_id() {
	char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	char rand_id[9];

	for (int i = 0; i < sizeof(rand_id) - 1; i++) {
		srand(time(NULL) + i);
		rand_id[i] = charset[rand() % (int)sizeof(charset)];
	}
	rand_id[8] = '\0';
	asprintf(&id, "%s", rand_id);
}

void create_sock(char *path) {
	asprintf(&sock_path, SOCK_PATH_FORMAT, path, id);

	if (fopen(sock_path, "r") != NULL) {
		return;
	}

	server_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un sockaddr = {0};

	if (server_sock_fd == -1) {
		return;
	}

	sockaddr.sun_family = AF_UNIX;
	strcpy(sockaddr.sun_path, sock_path);

	int ret = bind(server_sock_fd, (struct sockaddr*) &sockaddr, sizeof(struct sockaddr_un));

	if (ret == -1) {
		return;
	};
}

int main(int argc, char **argv) {
	if (argc > 1) {
		if (is_alphanumeric(argv[1]) && strlen(argv[1]) <= ID_CHAR_LIMIT) {
			id = argv[1];
		} else {
			fprintf(stderr, "Warning: Value is not alphanumeric or longer than %d characters, ignoring.\n", ID_CHAR_LIMIT);
			randomize_id();
		}
	} else {
		randomize_id();
	}

	char *runtime_dir = getenv("XDG_RUNTIME_DIR");

	if (runtime_dir == NULL) {
		fprintf(stderr, "Error: Environment variable XDG_RUNTIME_DIR is NULL, quitting.\n");
		return 1;
	}

	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);

	char *sock_dir;
	asprintf(&sock_dir, "%s/rtse", runtime_dir);
	
	mkdir(sock_dir, 0755);

	create_sock(runtime_dir);
	while (fopen(sock_path, "r") != NULL) {
		randomize_id();
		create_sock(runtime_dir);
	}

	if (listen(server_sock_fd, 128) == -1) {
		fprintf(stderr, "Error: Socket is already in use.\n");
		return 1;
	}

	printf("Created socket at %s (id: %s)\n", sock_path, id);

	for (;;) {
		printf("> ");
		fflush(stdout);

		accept_sock_fd = accept(server_sock_fd, NULL, NULL);

		char input_buffer[BUFFER_SIZE];
		read(accept_sock_fd, input_buffer, BUFFER_SIZE);

		printf("%s\n", input_buffer);
		
		int ret = system(input_buffer);

		if (WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) {
			break;
		}
	}
}
