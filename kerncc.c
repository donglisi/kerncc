#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <utils.h>

static char gcc[] = "gcc";
static char *cc;
static int value_size;
static int balance;

static void __attribute__ ((constructor)) __init__cc(void)
{
	if (getenv("KERNCC"))
		cc = getenv("KERNCC");
	else
		cc = gcc;

	if (getenv("KERNCC_SIZE"))
		value_size = atoi(getenv("KERNCC_SIZE"));
	else
		value_size = 1000;

	if (getenv("KERNCC_BALANCE"))
		balance = atoi(getenv("KERNCC_BALANCE"));
	else
		balance = 55;
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

static int get_sockfd()
{
	int sockfd;
	struct sockaddr_in serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error : Could not create socket\n");
		return -1;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000);

	if (inet_pton(AF_INET, "192.168.1.2", &serv_addr.sin_addr) <= 0) {
		printf("inet_pton error occured\n");
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Error : Connect Failed\n");
		return -1;
	}

	return sockfd;
}

static int remote_cc(int argc, char **argv)
{
	int sockfd, fd, es, ret = 0;
	char *opath, *dpath, *cmd, **args;

	sockfd = get_sockfd();
	if (sockfd == -1)
		return native_cc(argc, argv);

	write_from_str(sockfd, getenv("PWD"));

	cmd = get_cmd(argc, argv);
	args = argc_argv_to_args(argc, argv);

	write_from_str(sockfd, cmd);

	read(sockfd, &es, sizeof(int));
	if (es) {
		ret = native_cc(argc, argv);
		goto remote_compile_error;
	}

	get_opath(args, &opath);
	fd = open(opath, O_CREAT | O_WRONLY, 0644);
	read_to_fd(sockfd, fd);
	close(fd);

	get_dpath(args, &dpath);
	fd = open(dpath, O_CREAT | O_WRONLY, 0644);
	read_to_fd(sockfd, fd);
	close(fd);
	free(dpath);

remote_compile_error:
	free(cmd);
	close(sockfd);

	return ret;
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

	if (cpath[0] != '/')
		return false;

	fp = fopen("./files", "r");
	if (fp == NULL)
		return true;
	while (getline(&line, &len, fp) != -1) {
		len = strlen(line);
		line[len - 1] = 0;
		if (strstr(cpath, line)) {
			return false;
		}
	}
	fclose(fp);

	return true;
}
