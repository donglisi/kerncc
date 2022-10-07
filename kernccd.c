#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <linux/err.h>
#include <utils.h>

static int native_cc(int connfd, char **args)
{
	int wstatus, es, n;
	pid_t pid;

	pid = fork();

	if (!pid) {
		dup2(open("/dev/null", O_WRONLY, 0644), STDERR_FILENO);
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
	int connfd = *((int *)arg), i, fd, argc;
	char *cmd, **args, *kbdir, *odir, *opath, *dpath;

	kbdir = read_to_str(connfd);
	if (IS_ERR(kbdir))
		return 0;
	chdir(kbdir);

	cmd = read_to_str(connfd);
	if (IS_ERR(cmd))
		return 0;

	args = get_args(cmd);
	argc = get_argc(args);
	opath = args[argc - 2];

	dirname1(opath, &odir);
	mkdir_recursion(odir);

	if (native_cc(connfd, args))
		goto error;

	if (write_file_to_sockfd(connfd, opath))
		goto error;

	get_dpath(args, &dpath);
	write_file_to_sockfd(connfd, dpath);
	free(dpath);

error:
	for (i = 0; args[i]; i++)
		free(args[i]);
	free(args);
	free(arg);
	free(odir);
	free(cmd);
	free(kbdir);
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
