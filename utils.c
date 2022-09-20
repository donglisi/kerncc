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
	int n = 0, len, loc = 0;
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

int read_to_fd(int infd, int outfd)
{
	int n, size;
	char buf[BUFSIZ];

	n = read(infd, &size, sizeof(int));
	if (n < 0)
		return -1;

	while ((n = read(infd, buf, BUFSIZ < size ? BUFSIZ : size)) > 0) {
		if (n < 0)
			return -1;
		size -= n;
		if (write(outfd, buf, n) != n)
			printf("write error %d\n", n);
	}

	return 0;
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

void basename1(char *path, char **name)
{
	char *copy, *tmp;

	copy = malloc(strlen(path) + 1);
	strcpy(copy, path);
	tmp = basename(copy);
	*name = malloc(strlen(tmp) + 1);
	strcpy(*name, tmp);
	free(copy);
}

void mkdir_recursion(char *dir)
{
	char *cmd, mk[] = "mkdir -p ";
	int mk_len = strlen(mk), cmd_len = mk_len + strlen(dir) + 1;

	cmd = malloc(cmd_len);
	strcpy(cmd, mk);
	strcpy(&cmd[mk_len], dir);
	cmd[cmd_len -1 ] = 0;
	system(cmd);
	free(cmd);
}
