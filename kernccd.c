#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <zlib.h>
#include <string.h>
#include <pthread.h>

#include <linux/err.h>
#include <utils.h>
#include "zpipe.h"

static int native_cc(int connfd, char **args, char *ipath, char *opath)
{
	int wstatus, es, n, argc;
	pid_t pid;

	pid = fork();

	if (!pid) {
		dup2(open("/dev/null", O_WRONLY, 0644), STDERR_FILENO);
		argc = get_argc(args);
		args[argc - 1] = ipath;
		args[argc - 2] = opath;
		execvp(args[0], args);
	}

	waitpid(pid, &wstatus, 0);
	WIFEXITED(wstatus);
	es = WEXITSTATUS(wstatus);
	n = write(connfd, &es, sizeof(int));
	if (n < 0)
		return -1;

	/* if compile faile */
	if (es) {
		print_cpath(args);
		return -1;
	}

	return 0;
}

static void *cc_thread(void *arg)
{
	int connfd = *((int *)arg);
	char *cmd, **args, *opath, *ipath, *izpath;

	cmd = read_to_str(connfd);
	if (IS_ERR(cmd))
		goto cmd_error;

	args = get_args(cmd);
	ipath = get_ipath();
	opath = get_opath(ipath);
	izpath = get_izpath(ipath);

	if (read_file_from_sockfd(connfd, izpath))
		goto error;

	if (decompression(izpath, ipath))
		goto error;

	if (native_cc(connfd, args, ipath, opath))
		goto error;

	write_file_to_sockfd(connfd, opath);

error:
	remove(ipath);
	remove(izpath);
	remove(opath);
	free_args(args);
	free(ipath);
	free(opath);
	free(izpath);
	free(args);
	free(cmd);

cmd_error:
	free(arg);
	close(connfd);

	return 0;
}

int main(int argc, char *argv[])
{
	int listenfd, *connfd, option = 1;
	struct sockaddr_in serv_addr;
	pthread_t t;

	signal(SIGPIPE, SIG_IGN);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000);

	bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	listen(listenfd, 64);

	while (1) {
		connfd = malloc(sizeof(int));
		*connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
		pthread_create(&t, NULL, cc_thread, connfd);
	}
}
