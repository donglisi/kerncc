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

char *get_cmd(int connfd)
{
	int fd, tmp_len = 13, uuid_len = 37, path_len = tmp_len -1 + uuid_len -1 + 3;
	char tmp[tmp_len], uuid[uuid_len], *cmd, *full_cmd, opath[path_len], ipath[path_len];

	cmd = read_to_str(connfd);
	if (IS_ERR(cmd))
		return ERR_PTR(-EIO);

	strcpy(tmp, "/dev/shm/tmp");
	fd = open("/proc/sys/kernel/random/uuid", O_RDONLY);
	read(fd, uuid, uuid_len - 1);
	close(fd);
	uuid[uuid_len - 1] = 0;

	strcpy(opath, tmp);
	strcat(opath, uuid);
	strcat(opath, ".o");

	strcpy(ipath, tmp);
	strcat(ipath, uuid);
	strcat(ipath, ".i");

	full_cmd = malloc(strlen(cmd) + 1 + strlen(opath) + 1 + strlen(ipath) + 1);
	strcpy(full_cmd, cmd);
	strcat(full_cmd, " ");
	strcat(full_cmd, opath);
	strcat(full_cmd, " ");
	strcat(full_cmd, ipath);

	free(cmd);

	return full_cmd;
}

static void *cc_thread(void *arg)
{
	int connfd = *((int *)arg), i, argc, len, ret;
	char *cmd, **args, *opath, *ipath, *izpath;
	FILE *ifile, *izfile;

	cmd = get_cmd(connfd);
	if (IS_ERR(cmd))
		goto cmd_error;

	args = get_args(cmd);
	argc = get_argc(args);
	ipath = args[argc - 1];
	opath = args[argc - 2];

	len = strlen(ipath);
	izpath = malloc(len + 3);
	strcpy(izpath, ipath);
	strcat(izpath, ".z");

	if (read_file_from_sockfd(connfd, izpath))
		goto error;

	ifile = fopen(ipath, "w");
	izfile = fopen(izpath, "r");
	ret = inf(izfile, ifile);
	if (ret != Z_OK)
		zerr(ret);
	fclose(ifile);
	fclose(izfile);

	if (native_cc(connfd, args))
		goto error;

	write_file_to_sockfd(connfd, opath);

error:
	free(izpath);
	// remove(ipath);
	// remove(izpath);
	// remove(opath);
	for (i = 0; args[i]; i++)
		free(args[i]);
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
