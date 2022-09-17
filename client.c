#include <sys/socket.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "utils.h"

extern char cc[];

bool check_is_cc(int argc, char **argv)
{
	int i;

	if (!EndsWith(argv[argc - 1], ".c"))
		return false;

/*
	if (strstr(argv[argc - 1], "arch/x86") != NULL)
		return false;
*/

	for (i = 0; i < argc; i++) {
		if (!strcmp("-c", argv[i])) {
			return true;
		}
	}
	return false;
}

void gcc(char argc, char **argv)
{
	char *args[argc + 1];

	for (int i = 1; i < argc; i++) {
		args[i] = malloc(strlen(argv[i]) + 1);
		strcpy(args[i], argv[i]);
	}
	args[0] = cc;
	args[argc] = NULL;

	execvp(args[0], args);
}

int get_sockfd()
{
	int sockfd = 0;
	struct sockaddr_in serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error : Could not create socket\n");
		return -1;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000);

	if (inet_pton(AF_INET, "192.168.1.3", &serv_addr.sin_addr) <= 0) {
		printf("inet_pton error occured\n");
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Error : Connect Failed\n");
		return -1;
	}

	return sockfd;
}

void distcc(int argc, char **argv)
{
	int sockfd, i, fd, n, len;
	char buf[BUFSIZ], *opath, *dpath, *cmd, **args;

	sockfd = get_sockfd();
	if (sockfd == -1)
		gcc(argc, argv);

	cmd = get_cmd(argc, argv);

	write_from_str(sockfd, cmd);

	get_opath(argc, argv, &opath);
	fd = open(opath, O_CREAT | O_WRONLY, 0644);
	read_to_fd(sockfd, fd);
	close(fd);

	get_dpath(argc, argv, &dpath);
	fd = open(dpath, O_CREAT | O_WRONLY, 0644);
	read_to_fd(sockfd, fd);
	close(fd);

	free(cmd);
}

bool need_remote_cc(int argc, char **argv)
{
	if (check_is_cc(argc, argv)) {
		if (get_file_size(argv[argc - 1]) > 1000) {
			srand(time(NULL) + getpid());
			if (rand() % 8 > 2)
				return true;
			else
				return false;
		}
	}

	return false;
}

int main(int argc, char *argv[])
{
	if (need_remote_cc(argc, argv))
		distcc(argc, argv);
	else
		gcc(argc, argv);
	return 0;
}
