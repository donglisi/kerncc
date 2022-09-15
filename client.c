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

void gcc(char argc, char **argv)
{
	char *args[argc + 1];

	for (int i = 1; i < argc; i++) {
		args[i] = malloc(strlen(argv[i]) + 1);
		strcpy(args[i], argv[i]);
	}
	args[0] = "/usr/lib64/ccache/gcc";
	args[argc] = NULL;

	execvp(args[0], args);
}

int get_sockfd()
{
	int sockfd = 0;
	struct sockaddr_in serv_addr;

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

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("\n Error : Connect Failed \n");
		return 1;
	}

	return sockfd;
}

void distcc(int argc, char **argv)
{
	int sockfd, i, fd, n, iovs_len[argc];
	char buf[BUFSIZ], *opath;
	struct iovec *iovs;

	sockfd = get_sockfd();
	iovs = calloc(argc, sizeof(struct iovec));
	iovs[0].iov_base = "/usr/lib64/ccache/gcc";
	iovs[0].iov_len = strlen(iovs[0].iov_base) + 1;
	iovs_len[0] = iovs[0].iov_len;
	for (i = 1; i < argc; i++) {
		iovs[i].iov_base = argv[i];
		iovs[i].iov_len = strlen(argv[i]) + 1;
		iovs_len[i] = iovs[i].iov_len;
	}

	write(sockfd, &argc, sizeof(int));
	write(sockfd, iovs_len, sizeof(iovs_len));
	writev(sockfd, iovs, argc);

	get_opath(argc, argv, &opath);

	fd = open(opath, O_CREAT | O_WRONLY, 0644);
	while ((n = read(sockfd, buf, BUFSIZ)) > 0) {
		if (write(fd, buf, n) != n)
			printf("write error %d\n", n);
	}
	close(fd);
}

int main(int argc, char *argv[])
{
	int fd;
	struct stat statbuf;

	fd = open("/home/d/linux/Makefile", O_RDONLY);
	fstat(fd, &statbuf);
	close(fd);

	// printf("%9jd\n", statbuf.st_size);

	distcc(argc, argv);
	return 0;
}
