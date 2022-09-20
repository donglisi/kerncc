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
	int fd, wstatus, es, n;
	pid_t pid;

	pid = fork();

	if (!pid) {
		execvp(args[0], args);
	}

	waitpid(pid, &wstatus, 0);
	WIFEXITED(wstatus);
	es = WEXITSTATUS(wstatus);
	n = write(connfd, &es, sizeof(int));
	if (n < 0) {
		printf("write es %d to connfd error\n", es);
		return -1;
	}

	/* if compile faile */
	if (es) {
		print_args(args);
		return -1;
	}

	return 0;
}

static int write_file_to_client(int connfd, char *path)
{
	int fd, size, rn, wn, ret = 0;
	char buf[BUFSIZ];

	size = get_file_size(path);
	if (write(connfd, &size, sizeof(int)) != sizeof(int))
		return -1;

	fd = open(path, O_RDONLY);
	while ((rn = read(fd, buf, BUFSIZ)) > 0) {
		size = rn;
		while ((wn = write(connfd, &buf[rn - size], size)) > 0) {
			if (wn < 0)
				return -1;
			size -= wn;
		}
	}
	close(fd);

	return 0;
}

static void *cc_thread(void *arg)
{
	int connfd = *((int *)arg), i, fd;
	char *cmd, **args, *kbdir, *odir, *opath, *dpath;

	kbdir = read_to_str(connfd);
	if (IS_ERR(kbdir)) {
		printf("read_to_str error kbdir\n");
		return 0;
	}
	chdir(kbdir);

	cmd = read_to_str(connfd);
	if (IS_ERR(cmd)) {
		printf("read_to_str error cmd\n");
		return 0;
	}
	args = get_args(cmd);

	get_opath(args, &opath);
	dirname1(opath, &odir);
	mkdir_recursion(odir);

	if (native_cc(connfd, args))
		goto error;

	if (write_file_to_client(connfd, opath))
		goto error;

	get_dpath(args, &dpath);
	write_file_to_client(connfd, dpath);
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
	int listenfd = 0, *connfd, option = 1;
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

	listen(listenfd, 10);

	while (1) {
		connfd = malloc(sizeof(int));
		*connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
		pthread_create(&t, NULL, cc_thread, connfd);
	}
}
