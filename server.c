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

void print_cmd(char **args)
{
	int i;
	char *arg;

	for (i = 0, arg = args[0]; args[i]; i++)
		printf("%s ", args[i]);
	printf("\n");
}

void *gcc(void *arg)
{
	int connfd = *((int *)arg), fd, n, *iovs_len, iovcnt, size;
	char buf[BUFSIZ], **args, *opath, *dpath;
	struct iovec *iovs;
	pid_t pid;
	int wstatus;

	n = read(connfd, &iovcnt, sizeof(int));
	iovs_len = calloc(iovcnt, sizeof(int));
	read(connfd, iovs_len, sizeof(int) * iovcnt);
	iovs = calloc(iovcnt, sizeof(struct iovec));
	args = calloc(iovcnt + 1, sizeof(void*));
	for (int i = 0; i < iovcnt; i++) {
		iovs[i].iov_base = malloc(iovs_len[i]);
		iovs[i].iov_len = iovs_len[i];
		args[i] = iovs[i].iov_base;
	}
	readv(connfd, iovs, iovcnt);

	pid = fork();

	print_cmd(args);
	if (!pid) {
		execvp(args[0], args);
	}

	waitpid(pid, &wstatus, 0);

	get_opath(iovcnt, args, &opath);
	size = get_file_size(opath);
	write(connfd, &size, sizeof(int));
	fd = open(opath, O_RDONLY);
	while ((n = read(fd, buf, BUFSIZ)) > 0) {
		if (write(connfd, buf, n) != n)
			printf("write error\n");
	}
	close(fd);

	get_dpath(iovcnt, args, &dpath);
	fd = open(dpath, O_RDONLY);
	free(dpath);
	while ((n = read(fd, buf, BUFSIZ)) > 0)
		if (write(connfd, buf, n) != n)
			printf("write error\n");
	close(fd);

	close(connfd);

	for (int i = 0; i < iovcnt; i++)
		free(iovs[i].iov_base);
	free(args);
	free(iovs);
	free(iovs_len);
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
