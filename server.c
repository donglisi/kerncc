#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

int main(int argc, char *argv[])
{
	int listenfd = 0, connfd = 0, fd, n, *iovs_len, iovcnt;
	struct sockaddr_in serv_addr;
	char buf[BUFSIZ], **args;
	struct iovec *iovs;

	char sendBuff[1025];
	time_t ticks;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(sendBuff, '0', sizeof(sendBuff));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000);

	int option = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	listen(listenfd, 10);

	while (1) {
		connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
		read(connfd, &iovcnt, 4);
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

		fd = open("/home/d/linux/build/kernel/events/core.o", O_RDONLY);
		while ((n = read(fd, buf, BUFSIZ)) > 0)
			if (write(connfd, buf, n) != n)
				printf("write error\n");
		close(fd);
		close(connfd);
	}
}
