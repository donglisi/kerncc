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
	char *cmd, *full_cmd, **args, *opath, *ipath;

	opath = "/dev/shm/a.o";
	ipath = "/dev/shm/a.i";

	cmd = read_to_str(connfd);
	if (IS_ERR(cmd))
		return 0;
	full_cmd = malloc(strlen(cmd) + 1 + strlen(opath) + 1 + strlen(ipath) + 1);
	strcpy(full_cmd, cmd);
	strcat(full_cmd, " ");
	strcat(full_cmd, opath);
	strcat(full_cmd, " ");
	strcat(full_cmd, ipath);
	args = get_args(full_cmd);
	argc = get_argc(args);

	read_file_from_server(connfd, ipath);

/*
	opath = args[argc - 2];
*/

	if (native_cc(connfd, args))
		goto error;

	write_file_to_client(connfd, opath);

error:
	for (i = 0; args[i]; i++)
		free(args[i]);
	free(args);
	free(arg);
	free(cmd);
	free(full_cmd);
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
