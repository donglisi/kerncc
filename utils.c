#include <sys/socket.h>
#include <netinet/in.h>
#include <libgen.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <time.h>

#include "utils.h"

// char cc[] = "/usr/bin/gcc";
char cc[] = "/usr/lib64/ccache/gcc";

char *get_cmd(int argc, char **argv)
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

char **get_args(char *cmd)
{
	int i, j, len, loc, argc, cmd_len, *arg_lens;
	char **args, *arg;

	cmd_len = strlen(cmd);
	for (i = 0, argc = 0; i < cmd_len; i++)
		if (cmd[i] == ' ')
			argc++;
	argc++;

	arg_lens = calloc(argc, sizeof(int));
	for (i = 0, j = 0, len = 0 ; i < cmd_len; i++, len++) {
		if (cmd[i] == ' ') {
			if (j == 0)
				arg_lens[j] = len;
			else
				arg_lens[j] = len - 1;
			j++;
			len = 0;
		}
	}
	arg_lens[j] = len - 1;

	args = calloc(argc + 1, sizeof(void *));
	for (i = 0, loc = 0; i < argc; i++) {
		len = arg_lens[i];
		args[i] = malloc(len + 1);
		strncpy(args[i], &cmd[loc], len);
		args[i][len] = 0;
		loc += len + 1;
	}

	free(arg_lens);
	args[argc] = 0;

	return args;
}

void get_opath(char **args, char **opath)
{
	int i;

	for (i = 0; args[i]; i++) {
		if (!strcmp("-o", args[i])) {
			*opath = args[i + 1];
			return;
		}
	}
}

void get_dpath(char **args, char **dpath)
{
	int loc;
	char *opath, *dirpath, *name;

	get_opath(args, &opath);
	dirname1(opath, &dirpath);
	basename1(opath, &name);

	*dpath = malloc(strlen(opath) + 4);
	strcpy(*dpath, dirpath);
	loc = strlen(dirpath);
	strcpy(&((*dpath)[loc]), "/.");
	loc += 2;
	strcpy(&((*dpath)[loc]), name);
	loc += strlen(name);
	strcpy(&((*dpath)[loc]), ".d");
	loc += 2;
	(*dpath)[loc] = 0;

	free(dirpath);
	free(name);
}

void get_epath(char **args, char **epath)
{
	int len;
	char *opath;

	len = strlen(opath);
	get_opath(args, &opath);
	*epath = malloc(len + 3);
	strcpy(*epath, opath);
	strcpy(&((*epath)[len]), ".e");
	(*epath)[len + 2] = 0;
}

int get_file_size(char *path)
{
	int fd;
	struct stat statbuf;

	fd = open(path, O_RDONLY);
	fstat(fd, &statbuf);
	close(fd);

	return statbuf.st_size;
}

int EndsWith(const char *str, const char *suffix)
{
	if (!str || !suffix)
		return 0;
	size_t lenstr = strlen(str);
	size_t lensuffix = strlen(suffix);
	if (lensuffix >  lenstr)
		return 0;
	return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

char *read_to_str(int fd)
{
	int n, len, loc = 0;
	char *str, buf[BUFSIZ];

	read(fd, &len, sizeof(int));
	str = malloc(len);
	while ((n = read(fd, buf, BUFSIZ < len ? BUFSIZ : len)) > 0) {
		strncpy(&str[loc], buf, n);
		loc += n;
		len -= n;
	}
	return str;
}

void write_from_str(int fd, char *str)
{
	int n, len = strlen(str) + 1, loc = 0;

	write(fd, &len, sizeof(int));
	do {
		n = write(fd, &str[loc], len);
		loc += n;
		len -= n;
	} while (len > 0);
}

void read_to_fd(int infd, int outfd)
{
	int n, size;
	char buf[BUFSIZ];

	n = read(infd, &size, sizeof(int));
	while ((n = read(infd, buf, BUFSIZ < size ? BUFSIZ : size)) > 0) {
		size -= n;
		if (write(outfd, buf, n) != n)
			printf("write error %d\n", n);
	}
}

void print_args(char **args)
{
	int i;
	char *arg;

	for (i = 0, arg = args[0]; args[i]; i++)
		printf("%s ", args[i]);
	printf("\n");
}

void mkdir_recursion(char *path)
{
	char *cmd, mk[] = "mkdir -p ";
	int mk_len = strlen(mk), cmd_len = mk_len + strlen(path) + 1;

	cmd = malloc(cmd_len);
	strcpy(cmd, mk);
	strcpy(&cmd[mk_len], path);
	cmd[cmd_len -1 ] = 0;
	system(cmd);
	free(cmd);
}

void dirname1(char *path, char **dir)
{
	char *copy, *tmp;

	copy = malloc(strlen(path) + 1);
	strcpy(copy, path);
	tmp = dirname(copy);
	*dir = malloc(strlen(tmp) + 1);
	strcpy(*dir, tmp);
	free(copy);
}

char *basename1(char *path, char **name)
{
	char *copy, *tmp;

	copy = malloc(strlen(path) + 1);
	strcpy(copy, path);
	tmp = basename(copy);
	*name = malloc(strlen(tmp) + 1);
	strcpy(*name, tmp);
	free(copy);
}

char **argc_argv_to_args(char argc, char **argv)
{
	char **args;

	args = calloc(sizeof(void*), argc + 1);
	for (int i = 0; i < argc; i++)
		args[i] = argv[i];
	args[argc] = NULL;
	return args;
}

