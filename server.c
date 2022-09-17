#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <time.h>

#include "utils.h"

void *gcc(void *arg)
{
	int connfd = *((int *)arg), fd, n, len, argc;
	char buf[BUFSIZ], *cmd, **args, *opath, *dpath;
	pid_t pid;
	int wstatus;

	cmd = read_to_str(connfd);
	args = get_args(cmd);
	print_cmd(args);

	argc = get_argc(args);

	return 0;

	pid = fork();
	if (!pid) {
		execvp(args[0], args);
	}
	waitpid(pid, &wstatus, 0);

	for (int i = 0; i < argc; i++)
		free(args[i]);
	free(args);
	free(arg);
}

int main(int argc, char *argv[])
{
	int listenfd = 0, *connfd, option = 1;
	struct sockaddr_in serv_addr;
	pthread_t t;

	chdir("/home/d/linux");
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000);

	bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	listen(listenfd, 10);

	while (1) {
		connfd = malloc(sizeof(int));
		*connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
		pthread_create(&t, NULL, gcc, connfd);
	}
}
