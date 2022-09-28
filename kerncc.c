#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <zlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <utils.h>
#include "zpipe.h"

static char gcc[] = "gcc";
static char *cc;
static char server_ip[] = "192.168.1.2";
static int value_size = 1000;
static int balance = 50;

static void __attribute__ ((constructor)) __parse_env(void)
{
	if (getenv("KERNCC_CC"))
		cc = getenv("KERNCC_CC");
	else
		cc = gcc;

	if (getenv("KERNCC_IP"))
		strcpy(server_ip, getenv("KERNCC_IP"));

	if (getenv("KERNCC_SIZE"))
		value_size = atoi(getenv("KERNCC_SIZE"));

	if (getenv("KERNCC_BALANCE"))
		balance = atoi(getenv("KERNCC_BALANCE"));
}

static char *get_cmd(int argc, char **argv)
{
	int i, loc, size;
	char *cmd;

	size = strlen(cc) + 1;
	for (i = 1; i < argc; i++)
		size += strlen(argv[i]) + 1;

	cmd = malloc(size);
	strcpy(cmd, cc);
	loc = strlen(cc);
	cmd[loc++] = ' ';
	for (i = 1; i < argc; i++) {
		strcpy(&cmd[loc], argv[i]);
		loc += strlen(argv[i]);
		cmd[loc++] = ' ';
	}
	cmd[size - 1] = 0;

	return cmd;
}

static bool check_is_cc(int argc, char **argv);
static int native_cc(int argc, char **argv);

static bool need_remote_cc(int argc, char **argv)
{
	if (check_is_cc(argc, argv)) {
		if (get_file_size(argv[argc - 1]) > value_size) {
			srand(time(NULL) + getpid());
			if (rand() % 100 > balance)
				return true;
		}
	}

	return false;
}

static int get_sockfd(void)
{
	int sockfd;
	struct sockaddr_in serv_addr;

	signal(SIGPIPE, SIG_IGN);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000);

	if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
		return -1;

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		return -1;

	return sockfd;
}

static int native_cpp(int argc, char **argv, char *ipath)
{
	int wstatus, es, len;
	pid_t pid;
	char **args;

	pid = fork();
	if (!pid) {
		dup2(open("/dev/null", O_WRONLY, 0644), STDERR_FILENO);

		args = argc_argv_to_args(argc, argv);
		args[0] = "cpp";
		args[argc - 4][1] = 'E';
		args[argc - 2] = ipath;

		execvp(args[0], args);
	}

	waitpid(pid, &wstatus, 0);
	WIFEXITED(wstatus);
	es = WEXITSTATUS(wstatus);

	/* if compile faile */
	if (es) {
		print_cpath(args);
		return -1;
	}

	return 0;
}

static int remote_cc(int argc, char **argv)
{
	int sockfd, n, es = 1;
	char *cmd, *ipath, *izpath;

	sockfd = get_sockfd();
	if (sockfd == -1)
		return native_cc(argc, argv);

	cmd = get_cmd(argc - 2, argv);
	if (write_from_str(sockfd, cmd))
		return native_cc(argc, argv);

	ipath = get_ipath();
	if (native_cpp(argc, argv, ipath))
		return native_cc(argc, argv);

	izpath = get_izpath(ipath);
	if (compression(ipath, izpath))
		return native_cc(argc, argv);

	if (write_file_to_sockfd(sockfd, izpath))
		return native_cc(argc, argv);

	n = read(sockfd, &es, sizeof(int));
	if (n < 0 || es)
		return native_cc(argc, argv);

	if (read_file_from_sockfd(sockfd, argv[argc - 2]))
		return native_cc(argc, argv);

	free(cmd);
	close(sockfd);
	remove(ipath);
	remove(izpath);

	return 0;
}

static int native_cc(int argc, char **argv)
{
	char **args;

	args = argc_argv_to_args(argc, argv);
	args[0] = cc;

	return execvp(args[0], args);
}

int main(int argc, char *argv[])
{
	int ret;

	if (need_remote_cc(argc, argv))
		ret = remote_cc(argc, argv);
	else
		ret = native_cc(argc, argv);

	return ret;
}

static bool check_is_cc(int argc, char **argv)
{
	char *cpath = argv[argc - 1], *line;
	FILE *fp;
	size_t len = 0;

	if (!end_with(cpath, ".c"))
		return false;

	if (strcmp(argv[argc - 4], "-c"))
		return false;

	fp = fopen("./files", "r");
	if (fp == NULL)
		return true;
	while (getline(&line, &len, fp) != -1) {
		len = strlen(line);
		line[len - 1] = 0;
		if (!strcmp(cpath, line))
			return false;
	}
	fclose(fp);

	return true;
}
