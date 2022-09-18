#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "utils.h"

int verbose = 0;

void native_cc(int connfd, char **args)
{
	int fd, n, size, wstatus, es, erroutfd[2];
	char buf[BUFSIZ], *epath;
	pid_t pid;

	pipe(erroutfd);
	pid = fork();
	if (!pid) {
		close(erroutfd[0]);
		dup2(erroutfd[1], STDERR_FILENO);
		execvp(args[0], args);
	}
	close(erroutfd[1]);
	waitpid(pid, &wstatus, 0);

	WIFEXITED(wstatus);
	es = WEXITSTATUS(wstatus);

	/* if compile faile */
	if (es) {
		if (!verbose)
			print_args(args);

		write(connfd, &es, sizeof(int));

		get_epath(args, &epath);

		fd = open(epath, O_CREAT | O_RDWR, 0644);
		while ((n = read(erroutfd[1], buf, BUFSIZ)) > 0)
			if (write(fd, buf, n) != n)
				printf("write error\n");

		size = lseek(fd, 0, SEEK_END);
		write(connfd, &size, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		while ((n = read(fd, buf, BUFSIZ)) > 0)
			if (write(connfd, buf, n) != n)
				printf("write error\n");
		close(fd);

		remove(epath);
		free(epath);
	}

	close(erroutfd[0]);
}

void *cc_thread(void *arg)
{
	int connfd = *((int *)arg), i, fd, n, size;
	char buf[BUFSIZ], *cmd, **args, *dirpath, *opath, *dpath;

	cmd = read_to_str(connfd);
	if(verbose)
		printf("%s\n", cmd);

	args = get_args(cmd);

	get_opath(args, &opath);
	dirname1(opath, &dirpath);
	mkdir_recursion(dirpath);

	native_cc(connfd, args);

	size = get_file_size(opath);
	write(connfd, &size, sizeof(int));
	fd = open(opath, O_RDONLY);
	while ((n = read(fd, buf, BUFSIZ)) > 0)
		if (write(connfd, buf, n) != n)
			printf("write error\n");
	close(fd);

	get_dpath(args, &dpath);
	size = get_file_size(dpath);
	write(connfd, &size, sizeof(int));
	fd = open(dpath, O_RDONLY);
	while ((n = read(fd, buf, BUFSIZ)) > 0)
		if (write(connfd, buf, n) != n)
			printf("write error\n");
	close(fd);

	for (i = 0; args[i]; i++)
		free(args[i]);
	free(dpath);
	free(dirpath);
	free(args);
	free(arg);

	close(connfd);
}

int main(int argc, char *argv[])
{
	int listenfd = 0, *connfd, option = 1;
	char *pwd;
	struct sockaddr_in serv_addr;
	pthread_t t;

	if (argc > 1)
		verbose = 1;

	pwd = getenv("KBUILD_DIR");
	if (pwd)
		chdir(pwd);

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
		pthread_create(&t, NULL, cc_thread, connfd);
	}
}
