#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include <linux/err.h>
#include <utils.h>

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

void free_args(char **args)
{
	int i;

	for (i = 0; args[i]; i++)
		free(args[i]);
}

char **argc_argv_to_args(int argc, char **argv)
{
	int i;
	char **args;

	args = calloc(sizeof(void*), argc + 1);
	for (i = 0; i < argc; i++)
		args[i] = argv[i];
	args[argc] = NULL;

	return args;
}

int get_argc(char **args)
{
	int i;

	for (i = 0; args[i]; i++)
		;

	return i;
}

void print_argv(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");
}

void print_args(char **args)
{
	int i;

	for (i = 0; args[i]; i++)
		printf("%s ", args[i]);
	printf("\n");
}

void print_cpath(char **args)
{
	int argc;

	argc = get_argc(args);
	printf("%s\n", args[argc - 1]);
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

int end_with(const char *str, const char *suffix)
{
	size_t lenstr, lensuffix;

	if (!str || !suffix)
		return 0;

	lenstr = strlen(str);
	lensuffix = strlen(suffix);

	if (lensuffix >  lenstr)
		return 0;

	return !strncmp(str + lenstr - lensuffix, suffix, lensuffix);
}

char *read_to_str(int fd)
{
	int n, len, loc = 0;
	char *str, buf[BUFSIZ];

	n = read(fd, &len, sizeof(int));
	if (n != sizeof(int))
		return ERR_PTR(-EIO);

	str = malloc(len);
	while ((n = read(fd, buf, BUFSIZ < len ? BUFSIZ : len)) > 0) {
		strncpy(&str[loc], buf, n);
		loc += n;
		len -= n;
	}
	if (n < 0)
		return ERR_PTR(-EIO);

	return str;
}

int write_from_str(int fd, char *str)
{
	int n, len, loc = 0;

	len = strlen(str) + 1;
	n = write(fd, &len, sizeof(int));
	if (n < 0)
		return -1;

	do {
		n = write(fd, &str[loc], len);
		if (n < 0)
			return -1;
		loc += n;
		len -= n;
	} while (len > 0);

	return 0;
}

int read_file_from_sockfd(int sockfd, char *path)
{
	int fd, n, size;
	char buf[BUFSIZ];

	n = read(sockfd, &size, sizeof(int));
	if (n < 0)
		return -1;

	fd = open(path, O_CREAT | O_WRONLY, 0644);
	while ((n = read(sockfd, buf, BUFSIZ < size ? BUFSIZ : size)) > 0) {
		if (n < 0)
			return -1;
		size -= n;
		if (write(fd, buf, n) != n)
			return -1;
	}
	close(fd);

	return 0;
}

int write_file_to_sockfd(int sockfd, char *path)
{
	int fd, size, rn, wn;
	char buf[BUFSIZ];

	size = get_file_size(path);
	if (write(sockfd, &size, sizeof(int)) != sizeof(int))
		return -1;

	fd = open(path, O_RDONLY);
	while ((rn = read(fd, buf, BUFSIZ)) > 0) {
		size = rn;
		while ((wn = write(sockfd, &buf[rn - size], size)) > 0) {
			if (wn < 0)
				return -1;
			size -= wn;
		}
	}
	close(fd);

	return 0;
}

char *get_ipath(void)
{
	int fd, uuid_len = 37;
	char tmp[] = "/tmp/kerncc-", *uuid, *ipath;

	uuid = malloc(uuid_len);
	fd = open("/proc/sys/kernel/random/uuid", O_RDONLY);
	read(fd, uuid, uuid_len - 1);
	close(fd);
	uuid[uuid_len - 1] = 0;

	ipath = malloc(strlen(tmp) + strlen(uuid) + 3);
	strcpy(ipath, tmp);
	strcat(ipath, uuid);
	strcat(ipath, ".i");
	free(uuid);

	return ipath;
}

char *get_opath(char *ipath)
{
	char *opath;

	opath = malloc(strlen(ipath) + 1);
	strcpy(opath, ipath);
	opath[strlen(ipath) - 1] = 'o';

	return opath;
}

char *get_izpath(char *ipath)
{
	char *izpath;

	izpath = malloc(strlen(ipath) + 3);
	strcpy(izpath, ipath);
	strcat(izpath, ".z");

	return izpath;
}
