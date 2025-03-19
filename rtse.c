#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "consts.h"
#include "strcheck.h"

char *sock_path;
int client_sock_fd;

int server_sock_fd;
int accept_sock_fd;

char* get_random_id() {
	char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static char rand_id[9];

	for (int i = 0; i < sizeof(rand_id) - 1; i++) {
		srand(time(NULL) + i);
		rand_id[i] = charset[rand() % (int)sizeof(charset)];
	}
	rand_id[8] = '\0';
	return rand_id;
}

void quit(int exit_code) {
	remove(sock_path);
	exit(exit_code);
}

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

	sockaddr.sun_family = AF_UNIX;
	strcpy(sockaddr.sun_path, sock_path);

	int ret = connect(client_sock_fd, (struct sockaddr*) &sockaddr, sizeof(struct sockaddr_un));
	if (ret == -1) {
		fprintf(stderr, "Error: No server found running with specified ID\n");
		// return;
		exit(1);
	}
}

void create_sock(char *path, char* id) {
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


void send_cmd(char* id, char* message) {
	char *runtime_dir = getenv("XDG_RUNTIME_DIR");

	if (strlen(id) > ID_CHAR_LIMIT) {
		fprintf(stderr, "Error: ID is longer than %d characters.\n", ID_CHAR_LIMIT);
		exit(1);
	}

	if (runtime_dir == NULL) {
		fprintf(stderr, "Error: Environment variable XDG_RUNTIME_DIR is NULL, quitting.\n");
		exit(1);
	}

	asprintf(&sock_path, SOCK_PATH_FORMAT, runtime_dir, id);

	connect_to_sock(sock_path);

	char* buffer = message;

	if (strlen(buffer) > BUFFER_SIZE) {
		fprintf(stderr, "Warning: Command (%lu characters long) is longer than %d characters.\n", strlen(buffer), BUFFER_SIZE);
	}

	write(client_sock_fd, buffer, strlen(buffer) + 1);

	printf("Queued command '%s' at server with ID %s\n", message, id);  // queueing wasnt an intended feature (idk how to get the sockets to not queue) but its cool so im keeping it
}

void listen_to_cmds(char *id) {
	char *runtime_dir = getenv("XDG_RUNTIME_DIR");

	if (runtime_dir == NULL) {
		fprintf(stderr, "Error: Environment variable XDG_RUNTIME_DIR is NULL, quitting.\n");
		exit(1);
	}

	signal(SIGINT, quit);
	signal(SIGQUIT, quit);

	char *sock_dir;
	asprintf(&sock_dir, "%s/rtse", runtime_dir);
	
	mkdir(sock_dir, 0755);

	if (id == NULL) {
		id = get_random_id();
	}

	while (access(sock_path, F_OK) == 0) {
		id = get_random_id();
		asprintf(&sock_path, SOCK_PATH_FORMAT, sock_dir, id);
	}

	create_sock(runtime_dir, id);

	if (listen(server_sock_fd, 128) == -1) {
		fprintf(stderr, "Error: Socket is already in use.\n");
		exit(1);
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

int main(int argc, char** argv) {
	if (strcmp(argv[1], "-s") == 0) {
		send_cmd(argv[2], argv[3]);
	}

	if (strcmp(argv[1], "-l") == 0) {
		if (argc < 3) {
			listen_to_cmds(NULL);
		}
		listen_to_cmds(argv[2]);
	}

	if (strcmp(argv[1], "-h") == 0) {
		printf("Please view the source code (https://github.com/kennyaja/rtse) instead as this is in VERY early development and I haven't added a help function.\n");
	}
}
