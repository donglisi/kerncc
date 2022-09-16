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

void get_opath(int argc, char **argv, char **opath)
{
	int i;

	for (i = 0; i < argc; i++) {
		if (!strcmp("-o", argv[i])) {
			*opath = argv[i + 1];
			return;
		}
	}
}

void set_opath(int argc, char **argv, char **opath)
{
	int i;

	for (i = 0; i < argc; i++) {
		if (!strcmp("-o", argv[i])) {
			argv[i + 1] = *opath;
			return;
		}
	}
}

void *gcc(void *arg)
{
	int connfd = *((int *)arg), fd, n, *iovs_len, iovcnt;
	char buf[BUFSIZ], **args, opath[] = "/dev/stdout", *opathp = opath;
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
	set_opath(iovcnt, args, &opathp);

	pid = fork();

	if (!pid) {
		execvp(args[0], args);
	}

	waitpid(pid, &wstatus, 0);
/*
	fd = open(opath, O_RDONLY);
	while ((n = read(fd, buf, BUFSIZ)) > 0)
		if (write(connfd, buf, n) != n)
			printf("write error\n");

	close(fd);
*/
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
