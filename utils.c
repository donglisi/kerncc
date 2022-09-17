#include <sys/socket.h>
#include <netinet/in.h>
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

extern char cc[];
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

void get_opath(int argc, char **argv, char **opath)
{
	int i;

	for (i = 0; i < argc; i++) {
		if (!strcmp("-o", argv[i])) {
			*opath = argv[i + 1];
			return;
		}
	}
}

void get_dpath(int argc, char **argv, char **dpath)
{
	int loc;
	char *opath, *name;

	get_opath(argc, argv, &opath);
	*dpath = malloc(strlen(opath) + 4);
	dpath[strlen(opath) + 3] = 0;

	strcpy(*dpath, opath);
	name = strrchr(opath, '/');
	loc = strlen(opath) - strlen(name);
	strcpy(&((*dpath)[loc + 1]), name);
	(*dpath)[loc] = '/';
	(*dpath)[loc + 1] = '.';
	strcpy(&((*dpath)[strlen(opath) + 1]), ".d");
	// printf("dpath %s\n", *dpath);
}

void get_cmdpath(int argc, char **argv, char **cmdpath)
{
	int loc;
	char *opath, *name;

	get_opath(argc, argv, &opath);
	*cmdpath = malloc(strlen(opath) + 6);
	cmdpath[strlen(opath) + 5] = 0;

	strcpy(*cmdpath, opath);
	name = strrchr(opath, '/');
	loc = strlen(opath) - strlen(name);
	strcpy(&((*cmdpath)[loc + 1]), name);
	(*cmdpath)[loc] = '/';
	(*cmdpath)[loc + 1] = '.';
	strcpy(&((*cmdpath)[strlen(opath) + 1]), ".cmd");
	// printf("cmdpath %s\n", *cmdpath);
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
