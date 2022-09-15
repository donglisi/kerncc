#include <sys/socket.h>
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

int main(int argc, char *argv[])
{
	int sockfd = 0, n = 0, fd, iovs_len[argc];
	char recvBuff[BUFSIZ];
	struct sockaddr_in serv_addr;
	struct stat statbuf;
	struct iovec *iovs;

	char *args[argc + 1];
	for (int i = 1; i < argc; i++) {
		args[i] = malloc(strlen(argv[i]) + 1);
		strcpy(args[i], argv[i]);
	}
	args[0] = "/usr/lib64/ccache/gcc";
	args[argc] = NULL;

	// execvp("/usr/lib64/ccache/gcc", args);


	iovs = calloc(argc, sizeof(struct iovec));

	for (int i = 0; i < argc; i++) {
		iovs[i].iov_base = args[i];
		iovs[i].iov_len = strlen(args[i]) + 1;
		iovs_len[i] = iovs[i].iov_len;
	}

/*
	fd = open("/home/d/linux/Makefile", O_RDONLY);
	fstat(fd, &statbuf);
	close(fd);

	printf(" %9jd", statbuf.st_size);

	return 0;
*/

	memset(recvBuff, '0', sizeof(recvBuff));
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Error : Could not create socket \n");
		return 1;
	}


	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000);

	if (inet_pton(AF_INET, "192.168.1.3", &serv_addr.sin_addr) <= 0) {
		printf("\n inet_pton error occured\n");
		return 1;
	}

	if (connect
	    (sockfd, (struct sockaddr *) &serv_addr,
	     sizeof(serv_addr)) < 0) {
		printf("\n Error : Connect Failed \n");
		return 1;
	}

	write(sockfd, &argc, 4);
	write(sockfd, iovs_len, sizeof(int) * argc);
	writev(sockfd, iovs, argc);

	fd = open("/home/d/test/process.o", O_CREAT | O_WRONLY, 0644);
	while ((n = read(sockfd, recvBuff, BUFSIZ)) > 0) {
		if (write(fd, recvBuff, n) != n)
			printf("write error %d\n", n);
	}
	close(fd);

	return 0;
}
